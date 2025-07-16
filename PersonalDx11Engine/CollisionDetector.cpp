#include "CollisionDetector.h"
#include "ConfigReadManager.h"
#include "Debug.h"
#include "AABB.h"

#pragma region Configuration

FCollisionDetector::FCollisionDetector()
{
    LoadConfigFromIni();
}

void FCollisionDetector::LoadConfigFromIni()
{
    auto* configManager = UConfigReadManager::Get();
    if (!configManager)
    {
        LOG_WARNING("ConfigReadManager not available, using default collision detection values");
        return;
    }

    //configManager->GetValue("CCDTimeStep", CCDTimeStep);
    configManager->GetValue("MaxCCDIterations", MaxCCDIterations);
    configManager->GetValue("DistanceThreshold", DistanceThreshold);

    LOG_INFO("SIMD CollisionDetector configuration loaded:");
    //LOG_INFO("- CCD Time Step: %.4f", CCDTimeStep);
    LOG_INFO("- Max CCD Iterations: %d", MaxCCDIterations);
    LOG_INFO("- Distance Threshold: %.6f", DistanceThreshold);
}

#pragma endregion

#pragma region Main Detection Interface

FCollisionDetectionResult FCollisionDetector::DetectCollisionDiscrete(
    const FCollisionShapeData& shapeDataA,
    const FCollisionShapeData& shapeDataB)
{
    FCollisionDetectionResult result;

    // 동일한 형상끼리의 자가 충돌 방지
    if (&shapeDataA == &shapeDataB)
    {
        LOG_WARNING("DetectCollisionDiscrete: Attempted self-collision detection");
        return result;
    }

    // 형상 타입에 따른 특화된 SIMD 알고리즘 사용
    ECollisionShapeType typeA = shapeDataA.ShapeType;
    ECollisionShapeType typeB = shapeDataB.ShapeType;

    if (typeA == ECollisionShapeType::Sphere && typeB == ECollisionShapeType::Sphere)
    {
        // HalfExtent의 X 컴포넌트를 반지름으로 사용
        float radiusA = XMVectorGetX(shapeDataA.HalfExtent);
        float radiusB = XMVectorGetX(shapeDataB.HalfExtent);

        result = SphereSphere(
            radiusA, shapeDataA.CurrentWorldPosition,
            radiusB, shapeDataB.CurrentWorldPosition);
    }
    else if (typeA == ECollisionShapeType::Box && typeB == ECollisionShapeType::Box)
    {
        result = BoxBoxSAT(
            shapeDataA.HalfExtent, shapeDataA.CurrentWorldPosition, shapeDataA.CurrentWorldRotation,
            shapeDataB.HalfExtent, shapeDataB.CurrentWorldPosition, shapeDataB.CurrentWorldRotation);
    }
    else if (typeA == ECollisionShapeType::Box && typeB == ECollisionShapeType::Sphere)
    {
        float radiusB = XMVectorGetX(shapeDataB.HalfExtent);
        result = BoxSphereSimple(
            shapeDataA.HalfExtent, shapeDataA.CurrentWorldPosition, shapeDataA.CurrentWorldRotation,
            radiusB, shapeDataB.CurrentWorldPosition);
    }
    else if (typeA == ECollisionShapeType::Sphere && typeB == ECollisionShapeType::Box)
    {
        float radiusA = XMVectorGetX(shapeDataA.HalfExtent);
        result = BoxSphereSimple(
            shapeDataB.HalfExtent, shapeDataB.CurrentWorldPosition, shapeDataB.CurrentWorldRotation,
            radiusA, shapeDataA.CurrentWorldPosition);

        // 법선 방향 뒤집기 (A와 B가 바뀌었으므로)
        result.Normal = -result.Normal;
    }
    else
    {
        LOG_WARNING("DetectCollisionDiscrete: Unsupported shape combination (%d, %d)",
                    static_cast<int>(typeA), static_cast<int>(typeB));
        return result; // 충돌 없음으로 처리
    }

    // 결과 후처리
    if (result.bCollided)
    {
        result.TimeOfImpact = 1.0f; // 이산 충돌은 현재 프레임에서 발생

        // 법선 벡터 정규화 검증
        float normalLength = XMVector3Length(result.Normal).m128_f32[0];
        if (normalLength < KINDA_SMALL)
        {
            LOG_WARNING("DetectCollisionDiscrete: Invalid normal vector, using default");
            Vector3 normal = Vector3::Up();
            result.Normal = XMVectorSet(normal.x, normal.y, normal.z, 1.0f);
        }
        else if (std::abs(normalLength - 1.0f) > 0.01f)
        {
            result.Normal = result.Normal / normalLength;
        }

        // 침투 깊이 유효성 검증
        if (result.PenetrationDepth < 0.0f)
        {
            LOG_WARNING("DetectCollisionDiscrete: Negative penetration depth %.6f", result.PenetrationDepth);
            result.PenetrationDepth = 0.0f;
        }

        //LOG_NORMAL("SIMD Collision detected - Penetration: %.6f, Normal: (%.3f, %.3f, %.3f)",
        //          result.PenetrationDepth, result.Normal.m128_f32[0], result.Normal.m128_f32[1], result.Normal.m128_f32[2]);

    }

    return result;
}

FCollisionDetectionResult FCollisionDetector::DetectCollisionCCD(
    const FCollisionShapeData& shapeDataA,
    const FCollisionShapeData& shapeDataB,
    float deltaTime)
{
    FCollisionDetectionResult result;
    result.TimeOfImpact = deltaTime; // 기본값: 충돌 없음

    // Broad phase: Swept AABB 겹침 검사
    FMAABB sweptA = CalculateSweptAABB(shapeDataA);
    FMAABB sweptB = CalculateSweptAABB(shapeDataB);

    if (!sweptA.IsOverlapping(sweptB))
    {
        return result; // Swept AABB가 겹치지 않으면 충돌 없음
    }

    // 이진 탐색으로 충돌 시점 찾기
    float startTime = 0.0f;
    float endTime = 1.0f; // 정규화된 시간 (0~1)

    for (int iteration = 0; iteration < MaxCCDIterations; ++iteration)
    {
        float currentTime = startTime + (endTime - startTime) * 0.5f;

        // 해당 시점의 위치/회전 보간 (SIMD)
        XMVECTOR interpPosA = XMVectorLerp(shapeDataA.PrevWorldPosition, shapeDataA.CurrentWorldPosition, currentTime);
        XMVECTOR interpRotA = XMQuaternionSlerp(shapeDataA.PrevWorldRotation, shapeDataA.CurrentWorldRotation, currentTime);
        XMVECTOR interpPosB = XMVectorLerp(shapeDataB.PrevWorldPosition, shapeDataB.CurrentWorldPosition, currentTime);
        XMVECTOR interpRotB = XMQuaternionSlerp(shapeDataB.PrevWorldRotation, shapeDataB.CurrentWorldRotation, currentTime);

        // 보간된 위치에서 이산 충돌 검출
        FCollisionShapeData interpDataA = shapeDataA;
        FCollisionShapeData interpDataB = shapeDataB;
        interpDataA.CurrentWorldPosition = interpPosA;
        interpDataA.CurrentWorldRotation = interpRotA;
        interpDataB.CurrentWorldPosition = interpPosB;
        interpDataB.CurrentWorldRotation = interpRotB;

        FCollisionDetectionResult discreteResult = DetectCollisionDiscrete(interpDataA, interpDataB);

        if (discreteResult.bCollided)
        {
            result = discreteResult;
            endTime = currentTime; // 충돌 발견, 전반부에서 탐색
        }
        else
        {
            startTime = currentTime; // 충돌 없음, 후반부에서 탐색
        }
    }

    // 최종 충돌 시점 설정
    result.TimeOfImpact = endTime * deltaTime;

    if (result.bCollided)
    {
        LOG("SIMD CCD collision detected at time %.6f", result.TimeOfImpact);
    }

    return result;
}

#pragma endregion

#pragma region SIMD Shape-Based Detection Methods

FCollisionDetectionResult FCollisionDetector::SphereSphere(
    float radiusA, XMVECTOR positionA,
    float radiusB, XMVECTOR positionB)
{
    FCollisionDetectionResult result;

    // 중심점 간의 벡터 계산 (SIMD)
    XMVECTOR delta = XMVectorSubtract(positionB, positionA);
    XMVECTOR distanceSquaredVec = XMVector3LengthSq(delta);
    float distanceSquared = XMVectorGetX(distanceSquaredVec);

    float radiusSum = radiusA + radiusB;
    float radiusSumSquared = radiusSum * radiusSum;

    // 충돌 검사
    if (distanceSquared >= radiusSumSquared)
    {
        return result; // 충돌 없음
    }

    result.bCollided = true;

    XMVECTOR distanceVec = XMVector3Length(delta);
    float distance = XMVectorGetX(distanceVec);
    result.PenetrationDepth = radiusSum - distance;

    if (distance < KINDA_SMALL)
    {
        // 구체가 거의 같은 위치에 있는 경우
        result.Normal = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f); // Y축 방향으로 기본 설정
        result.Point = positionA;
    }
    else
    {
        // 정규화된 충돌 법선 계산 (A에서 B 방향) - SIMD
        XMVECTOR normal = XMVectorDivide(delta, distanceVec);
        result.Normal = normal;

        // 충돌 지점은 A 표면에서 법선 방향으로 반지름만큼 이동한 점
        XMVECTOR contactPoint = XMVectorAdd(positionA, XMVectorScale(normal, radiusA));
        result.Point = contactPoint;
    }

    return result;
}

FCollisionDetectionResult FCollisionDetector::BoxBoxSAT(
    XMVECTOR extentA, XMVECTOR positionA, XMVECTOR rotationA,
    XMVECTOR extentB, XMVECTOR positionB, XMVECTOR rotationB)
{
    FCollisionDetectionResult result;

    // SAT(Separating Axes Theorem)를 이용한 Box-Box 충돌 검출
    // 15개의 분리축을 검사: 각 박스의 3개 축 + 각 축들의 외적으로 생성된 9개 축

    // 회전 매트릭스 생성 (SIMD)
    XMMATRIX matrixA = CreateRotationMatrix(rotationA);
    XMMATRIX matrixB = CreateRotationMatrix(rotationB);

    // 박스 A의 로컬 축 벡터들 (월드 공간) - SIMD
    XMVECTOR axisA[3] = {
        XMVector3Normalize(matrixA.r[0]), // Right
        XMVector3Normalize(matrixA.r[1]), // Up  
        XMVector3Normalize(matrixA.r[2])  // Forward
    };

    // 박스 B의 로컬 축 벡터들 (월드 공간) - SIMD
    XMVECTOR axisB[3] = {
        XMVector3Normalize(matrixB.r[0]),
        XMVector3Normalize(matrixB.r[1]),
        XMVector3Normalize(matrixB.r[2])
    };

    // 박스 중심점 간의 벡터 (SIMD)
    XMVECTOR centerDelta = XMVectorSubtract(positionB, positionA);

    float minOverlap = FLT_MAX;
    XMVECTOR separatingAxis = XMVectorZero();

    // 각 분리축에 대해 검사하는 람다 함수
    auto TestSeparatingAxis = [&](XMVECTOR axis, bool flipNormal = false) -> bool
        {
            // 축 길이 검사
            XMVECTOR axisLengthVec = XMVector3Length(axis);
            float axisLength = XMVectorGetX(axisLengthVec);
            if (axisLength < KINDA_SMALL)
                return true; // 유효하지 않은 축, 분리되지 않음

            // 축 정규화
            axis = XMVectorDivide(axis, axisLengthVec);

            // 각 박스의 반 크기를 해당 축에 투영
            float projectedExtentA = 0.0f;
            float projectedExtentB = 0.0f;

            // 박스 A의 투영된 반 크기 계산
            for (int i = 0; i < 3; ++i)
            {
                XMVECTOR dotVec = XMVector3Dot(axis, axisA[i]);
                float projection = std::abs(XMVectorGetX(dotVec));
                float extent = XMVectorGetByIndex(extentA, i);
                projectedExtentA += extent * projection;
            }

            // 박스 B의 투영된 반 크기 계산
            for (int i = 0; i < 3; ++i)
            {
                XMVECTOR dotVec = XMVector3Dot(axis, axisB[i]);
                float projection = std::abs(XMVectorGetX(dotVec));
                float extent = XMVectorGetByIndex(extentB, i);
                projectedExtentB += extent * projection;
            }

            // 중심점 간의 거리를 해당 축에 투영
            XMVECTOR centerDistanceVec = XMVector3Dot(centerDelta, axis);
            float centerDistance = std::abs(XMVectorGetX(centerDistanceVec));

            // 분리 검사
            float totalExtent = projectedExtentA + projectedExtentB;
            if (centerDistance > totalExtent + DistanceThreshold)
            {
                return false; // 분리됨, 충돌 없음
            }

            // 겹침 깊이 계산
            float overlap = totalExtent - centerDistance;
            if (overlap < minOverlap)
            {
                minOverlap = overlap;
                separatingAxis = flipNormal ? XMVectorNegate(axis) : axis;
            }

            return true; // 이 축에서는 분리되지 않음
        };

    // 1. 박스 A의 3개 축 검사
    for (int i = 0; i < 3; ++i)
    {
        if (!TestSeparatingAxis(axisA[i]))
            return result; // 분리됨
    }

    // 2. 박스 B의 3개 축 검사
    for (int i = 0; i < 3; ++i)
    {
        if (!TestSeparatingAxis(axisB[i]))
            return result; // 분리됨
    }

    // 3. 각 축들의 외적으로 생성된 9개 축 검사
    for (int i = 0; i < 3; ++i)
    {
        for (int j = 0; j < 3; ++j)
        {
            XMVECTOR crossAxis = XMVector3Cross(axisA[i], axisB[j]);
            if (!TestSeparatingAxis(crossAxis))
                return result; // 분리됨
        }
    }

    // 모든 축에서 분리되지 않았으므로 충돌
    result.bCollided = true;
    result.PenetrationDepth = minOverlap;

    // 법선 벡터 설정 (A에서 B 방향)
    result.Normal = separatingAxis;

    // 충돌점 계산 (단순화: 두 박스 중심점의 중점)
    XMVECTOR contactPoint = XMVectorScale(XMVectorAdd(positionA, positionB), 0.5f);
    result.Point = contactPoint;

    return result;
}

FCollisionDetectionResult FCollisionDetector::BoxSphereSimple(
    XMVECTOR boxExtent, XMVECTOR boxPosition, XMVECTOR boxRotation,
    float sphereRadius, XMVECTOR spherePosition)
{
    FCollisionDetectionResult result;

    // 구의 중심을 박스의 로컬 공간으로 변환
    XMMATRIX boxMatrix = CreateTransformMatrix(boxPosition, boxRotation);
    XMMATRIX boxInverseMatrix = XMMatrixInverse(nullptr, boxMatrix);

    // 구 중심을 박스 로컬 공간으로 변환
    XMVECTOR localSphereCenter = XMVector3TransformCoord(spherePosition, boxInverseMatrix);

    // 박스의 로컬 공간에서 가장 가까운 점 찾기
    XMVECTOR closestPoint = XMVectorClamp(localSphereCenter, XMVectorNegate(boxExtent), boxExtent);

    // 구 중심에서 가장 가까운 점까지의 벡터 (로컬 공간)
    XMVECTOR localDelta = XMVectorSubtract(localSphereCenter, closestPoint);
    XMVECTOR distanceSquaredVec = XMVector3LengthSq(localDelta);
    float distanceSquared = XMVectorGetX(distanceSquaredVec);

    // 충돌 검사
    float radiusSquared = sphereRadius * sphereRadius;
    if (distanceSquared >= radiusSquared)
    {
        return result; // 충돌 없음
    }

    result.bCollided = true;

    // 거리와 침투 깊이 계산
    XMVECTOR distanceVec = XMVector3Length(localDelta);
    float distance = XMVectorGetX(distanceVec);
    result.PenetrationDepth = sphereRadius - distance;

    // 특별한 경우: 구 중심이 박스 내부에 있는 경우
    if (distance < KINDA_SMALL)
    {
        // 가장 가까운 박스 면으로의 방향 찾기
        XMVECTOR directions[6] = {
            XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), XMVectorSet(-1.0f, 0.0f, 0.0f, 0.0f), // X축
            XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), XMVectorSet(0.0f,-1.0f, 0.0f, 0.0f), // Y축
            XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), XMVectorSet(0.0f, 0.0f,-1.0f, 0.0f)  // Z축
        };

        float distances[6] = {
            XMVectorGetX(boxExtent) - XMVectorGetX(localSphereCenter), XMVectorGetX(boxExtent) + XMVectorGetX(localSphereCenter),
            XMVectorGetY(boxExtent) - XMVectorGetY(localSphereCenter), XMVectorGetY(boxExtent) + XMVectorGetY(localSphereCenter),
            XMVectorGetZ(boxExtent) - XMVectorGetZ(localSphereCenter), XMVectorGetZ(boxExtent) + XMVectorGetZ(localSphereCenter)
        };

        // 가장 가까운 면 찾기
        int closestFace = 0;
        float minDistance = distances[0];
        for (int i = 1; i < 6; ++i)
        {
            if (distances[i] < minDistance)
            {
                minDistance = distances[i];
                closestFace = i;
            }
        }

        // 로컬 법선 설정
        XMVECTOR localNormal = directions[closestFace];
        result.PenetrationDepth = sphereRadius + minDistance;

        // 로컬 법선을 월드 공간으로 변환
        XMVECTOR worldNormal = XMVector3TransformNormal(localNormal, boxMatrix);
        result.Normal = XMVector3Normalize(worldNormal);

        // 접촉점은 박스 표면의 해당 점
        XMVECTOR localContactPoint = XMVectorAdd(localSphereCenter, XMVectorScale(localNormal, minDistance));
        XMVECTOR worldContact = XMVector3TransformCoord(localContactPoint, boxMatrix);
        result.Point = worldContact;
    }
    else
    {
        // 일반적인 경우: 구가 박스 외부에서 접촉
        XMVECTOR localNormal = XMVectorDivide(localDelta, distanceVec);

        // 로컬 법선을 월드 공간으로 변환
        XMVECTOR worldNormal = XMVector3TransformNormal(localNormal, boxMatrix);
        result.Normal = XMVector3Normalize(worldNormal);

        // 접촉점은 박스 표면의 가장 가까운 점 (월드 공간)
        XMVECTOR worldContact = XMVector3TransformCoord(closestPoint, boxMatrix);
        result.Point = worldContact;
    }

    return result;
}

#pragma endregion

#pragma region SIMD Utility Methods

FMAABB FCollisionDetector::CalculateSweptAABB(const FCollisionShapeData& shapeData) const
{
    // 이전 위치와 현재 위치에서의 AABB를 계산하고 합치기
    FMAABB prevAABB = CalculateWorldAABB(shapeData, shapeData.PrevWorldPosition, shapeData.PrevWorldRotation);
    FMAABB currentAABB = CalculateWorldAABB(shapeData, shapeData.CurrentWorldPosition, shapeData.CurrentWorldRotation);

    return FMAABB::Merge(prevAABB, currentAABB);
}

FMAABB FCollisionDetector::CalculateWorldAABB(const FCollisionShapeData& shapeData,
                                              XMVECTOR position, XMVECTOR rotation) const
{
    FMAABB aabb;

    switch (shapeData.ShapeType)
    {
        case ECollisionShapeType::Sphere:
        {
            float radius = XMVectorGetX(shapeData.HalfExtent);
            XMVECTOR radiusVec = XMVectorReplicate(radius);

            XMVECTOR minVec = XMVectorSubtract(position, radiusVec);
            XMVECTOR maxVec = XMVectorAdd(position, radiusVec);

            aabb.vMin = minVec;
            aabb.vMax = maxVec;
        }
        break;

        case ECollisionShapeType::Box:
        {
            // 회전을 고려한 Box AABB 계산 (SIMD)
            XMMATRIX worldMatrix = CreateTransformMatrix(position, rotation);

            // 로컬 공간의 8개 코너 (SIMD)
            XMVECTOR extent = shapeData.HalfExtent;
            XMVECTOR localCorners[8] = {
                XMVectorSet(-XMVectorGetX(extent), -XMVectorGetY(extent), -XMVectorGetZ(extent), 1.0f),
                XMVectorSet(XMVectorGetX(extent), -XMVectorGetY(extent), -XMVectorGetZ(extent), 1.0f),
                XMVectorSet(-XMVectorGetX(extent),  XMVectorGetY(extent), -XMVectorGetZ(extent), 1.0f),
                XMVectorSet(XMVectorGetX(extent),  XMVectorGetY(extent), -XMVectorGetZ(extent), 1.0f),
                XMVectorSet(-XMVectorGetX(extent), -XMVectorGetY(extent),  XMVectorGetZ(extent), 1.0f),
                XMVectorSet(XMVectorGetX(extent), -XMVectorGetY(extent),  XMVectorGetZ(extent), 1.0f),
                XMVectorSet(-XMVectorGetX(extent),  XMVectorGetY(extent),  XMVectorGetZ(extent), 1.0f),
                XMVectorSet(XMVectorGetX(extent),  XMVectorGetY(extent),  XMVectorGetZ(extent), 1.0f)
            };

            // 첫 번째 코너로 초기화
            XMVECTOR worldCorner0 = XMVector3TransformCoord(localCorners[0], worldMatrix);
            XMVECTOR minVec = worldCorner0;
            XMVECTOR maxVec = worldCorner0;

            // 나머지 코너들로 AABB 확장 (SIMD)
            for (int i = 1; i < 8; ++i)
            {
                XMVECTOR worldCorner = XMVector3TransformCoord(localCorners[i], worldMatrix);
                minVec = XMVectorMin(minVec, worldCorner);
                maxVec = XMVectorMax(maxVec, worldCorner);
            }

            aabb.vMin = minVec;
            aabb.vMax = maxVec;
        }
        break;

        default:
            // 기본적으로 Box처럼 처리
        {
            XMVECTOR extent = shapeData.HalfExtent;
            XMVECTOR minVec = XMVectorSubtract(position, extent);
            XMVECTOR maxVec = XMVectorAdd(position, extent);

            aabb.vMin = minVec;
            aabb.vMax = maxVec;
        }
        break;
    }

    return aabb;
}

FMAABB FCollisionDetector::CalculateCurrentWorldAABB(const FCollisionShapeData& shapeData) const
{
    return CalculateWorldAABB(shapeData, shapeData.CurrentWorldPosition, shapeData.CurrentWorldRotation);
}

XMMATRIX FCollisionDetector::CreateRotationMatrix(XMVECTOR rotation) const
{
    // Quaternion에서 회전 매트릭스 생성
    return XMMatrixRotationQuaternion(rotation);
}

XMMATRIX FCollisionDetector::CreateTransformMatrix(XMVECTOR position, XMVECTOR rotation) const
{
    // 회전 매트릭스 생성
    XMMATRIX rotationMatrix = CreateRotationMatrix(rotation);

    // 평행이동 적용
    XMMATRIX translationMatrix = XMMatrixTranslationFromVector(position);

    // 회전 * 평행이동 순서로 결합
    return XMMatrixMultiply(rotationMatrix, translationMatrix);
}

#pragma endregion