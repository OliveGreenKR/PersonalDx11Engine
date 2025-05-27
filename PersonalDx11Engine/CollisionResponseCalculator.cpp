#include "CollisionResponseCalculator.h"
#include "VelocityConstraint.h"
#include "PhysicsStateInterface.h"
#include "Debug.h"
#include "PhysicsDefine.h"

FCollisionResponseResult FCollisionResponseCalculator::CalculateResponseByContraints(
    const FCollisionDetectionResult& DetectionResult,
    const FPhysicsParameters& ParameterA,
    const FPhysicsParameters& ParameterB,
    FAccumulatedConstraint& Accumulation,
    const float DeltaTime)
{
    FCollisionResponseResult ResponseData;

    // 충돌이 없을경우 바로 반환
    if (!DetectionResult.bCollided)
        return ResponseData;

    float currentNormalLambda = Accumulation.normalLambda;
    float currentFrictionLambda = Accumulation.frictionLambda;

    // 1. 법선 방향 충돌 제약 조건 해결 (위치 보정 포함)
    // 이 함수 내부에서 currentNormalLambda가 업데이트되고, 위치 보정 편향이 계산되어 속도에 반영됩니다.
    Vector3 NormalImpulse = SolveNormalCollisionConstraint(
        DetectionResult, ParameterA, ParameterB, Accumulation, DeltaTime, currentNormalLambda);

    // 2. 접선 방향 마찰 제약 조건 해결
    // 법선 임펄스(currentNormalLambda)는 마찰력 클램핑에 사용됩니다.
    Vector3 TangentImpulse = SolveFrictionConstraint(
        DetectionResult, ParameterA, ParameterB, Accumulation, currentNormalLambda, currentFrictionLambda);

    // 마찰에 대한 접선 방향 충격량 약화 계수 (기존 상수 유지)
    constexpr float TangentCoef = 1.0f;

    // 최종 순수 충격량 합산
    ResponseData.NetImpulse = (NormalImpulse + TangentCoef * TangentImpulse);
    ResponseData.ApplicationPoint = DetectionResult.Point;

    // 누적 람다값 업데이트 (SolveNormalCollisionConstraint와 SolveFrictionConstraint에서 이미 업데이트되지만, 명시적으로 다시 할당)
    Accumulation.normalLambda = currentNormalLambda;
    Accumulation.frictionLambda = currentFrictionLambda;

    return ResponseData;
}

void FCollisionResponseCalculator::ClampFriction(const float TangentRelativeVelocityLength,
                                                 const float NormalLambda,
                                                 const float StaticFriction,
                                                 const float KineticFriction,
                                                 float& OutFrictionLambda,
                                                 Vector3& OutTangentImpulse)
{
    // 정적 마찰과 운동 마찰 적용 (Coulomb Friction Model)
    float maxFrictionImpulse = NormalLambda; // 람다는 이미 스케일링된 값이라고 가정 (충격량 크기)

    if (TangentRelativeVelocityLength < KINDA_SMALL) // 상대 속도가 매우 작으면 정적 마찰 적용
    {
        maxFrictionImpulse *= StaticFriction;
    }
    else // 움직이고 있다면 운동 마찰 적용
    {
        maxFrictionImpulse *= KineticFriction;
    }

    // 마찰 람다를 계산된 최대 임펄스 값으로 클램핑
    OutFrictionLambda = Math::Clamp(OutFrictionLambda, -maxFrictionImpulse, maxFrictionImpulse);
    // 마찰 임펄스도 클램프
    float currentTangentImpulseMagnitude = OutTangentImpulse.Length();
    if (currentTangentImpulseMagnitude > maxFrictionImpulse)
    {
        OutTangentImpulse = OutTangentImpulse * (maxFrictionImpulse / currentTangentImpulseMagnitude);
    }
}


Vector3 FCollisionResponseCalculator::SolveNormalCollisionConstraint(
    const FCollisionDetectionResult& DetectionResult,
    const FPhysicsParameters& ParameterA,
    const FPhysicsParameters& ParameterB,
    FAccumulatedConstraint& Accumulation,
    float DeltaTime,
    float& OutNormalLambda)
{
    if (!DetectionResult.bCollided)
        return Vector3::Zero();

    // 기존 누적 람다값으로 시작 (Warm Starting)
    OutNormalLambda = Accumulation.normalLambda;

    Vector3 Normal = DetectionResult.Normal;
    FVelocityConstraint NormalConstraint(Normal, 0.0f, 0.0f);
    NormalConstraint.SetContactData(DetectionResult.Point, Normal, DetectionResult.PenetrationDepth);

    // 반발 계수 적용 (두 물체의 평균)
    float RestitutionCoef = std::min(0.9999f, (ParameterA.Restitution + ParameterB.Restitution) * 0.5f);

    // 상대 속도 계산
    XMVECTOR vRelativeVel = NormalConstraint.CalculateRelativeVelocity(
        ParameterA, ParameterB, XMLoadFloat3(&DetectionResult.Point));
    XMVECTOR vNormal = XMLoadFloat3(&Normal);
    float NormalVelocity = XMVectorGetX(XMVector3Dot(vRelativeVel, vNormal));

    // 속도가 임계값 이하면 반발 무시 (정지 상태 유지)
    const float VelocityThreshold = KINDA_SMALL;
    if (std::abs(NormalVelocity) < VelocityThreshold)
        RestitutionCoef = 0.0f;

    // 반발 속도 목표 설정
    float DesiredVelocity = NormalVelocity < 0.0f ? -NormalVelocity * RestitutionCoef : 0.0f;

    // 위치 오차 보정 계수 (Baumgarte beta 값)
    const float BiasFactor = 0.2f; // 일반적인 값 (0.1 ~ 0.3)

    // 위치 오류를 속도 편향으로 변환하여 DesiredVelocity에 추가
    float positionBiasVelocity = CalculatePositionBiasVelocity(
        DetectionResult.PenetrationDepth, BiasFactor, DeltaTime, 0.01f);
    DesiredVelocity += positionBiasVelocity;

    NormalConstraint.SetDesiredSpeed(DesiredVelocity);

    // 제약 조건 해결 (충격량 계산)
    Vector3 NormalImpulse = NormalConstraint.Solve(ParameterA, ParameterB, OutNormalLambda);
    Accumulation.normalLambda = OutNormalLambda; // 누적 람다 업데이트

    return NormalImpulse;
}

Vector3 FCollisionResponseCalculator::SolveFrictionConstraint(
    const FCollisionDetectionResult& DetectionResult,
    const FPhysicsParameters& ParameterA,
    const FPhysicsParameters& ParameterB,
    FAccumulatedConstraint& Accumulation,
    float InNormalLambda, // 법선 임펄스 크기 (마찰 클램핑에 사용)
    float& OutFrictionLambda)
{
    OutFrictionLambda = Accumulation.frictionLambda; // Warm Starting

    // 법선 방향 성분 제거하여 접선 방향 상대 속도 계산
    XMVECTOR vRelativeVel = FVelocityConstraint::CalculateRelativeVelocity(
        ParameterA, ParameterB, XMLoadFloat3(&DetectionResult.Point));

    XMVECTOR vNormal = XMLoadFloat3(&DetectionResult.Normal);
    float NormalVelocity = XMVectorGetX(XMVector3Dot(vRelativeVel, vNormal));

    XMVECTOR vNormalComponent = XMVectorScale(vNormal, NormalVelocity);
    XMVECTOR vTangentVel = XMVectorSubtract(vRelativeVel, vNormalComponent);

    float TangentLength = XMVectorGetX(XMVector3Length(vTangentVel));
    Vector3 TangentImpulse = Vector3::Zero();

    if (TangentLength > KINDA_SMALL)
    {
        // 접선 방향 정규화
        XMVECTOR vTangent = XMVectorDivide(vTangentVel, XMVectorReplicate(TangentLength));
        Vector3 Tangent;
        XMStoreFloat3(&Tangent, vTangent);

        // 접선 방향 제약 조건 (마찰력)
        // 마찰은 속도를 0으로 만드는 것이 목표이므로 DesiredSpeed는 0.0f
        FVelocityConstraint FrictionConstraint(Tangent, 0.0f);
        FrictionConstraint.SetContactData(DetectionResult.Point, DetectionResult.Normal, 0.0f);

        // Warm Starting: 이전 프레임의 마찰 람다 재사용
        TangentImpulse = FrictionConstraint.Solve(ParameterA, ParameterB, OutFrictionLambda);

        // 마찰력 크기 제한 (Coulomb 마찰 모델)
        float StaticFriction = (ParameterA.FrictionStatic + ParameterB.FrictionStatic) * 0.5f;
        float KineticFriction = (ParameterA.FrictionKinetic + ParameterB.FrictionKinetic) * 0.5f;

        // 마찰 클램핑
        ClampFriction(TangentLength, InNormalLambda, StaticFriction, KineticFriction,
                      OutFrictionLambda, TangentImpulse);

        Accumulation.frictionLambda = OutFrictionLambda; // 누적 람다 업데이트
    }

    return TangentImpulse;
}



float FCollisionResponseCalculator::CalculatePositionBiasVelocity(
    float PenetrationDepth,
    float BiasFactor,
    float DeltaTime,
    float Slop)
{
    Slop *= ONE_METER; //미터 단위로 변환

    // 슬롭(Slop)을 초과하는 침투만 고려
    float biasPenetration = fmaxf(0.0f, PenetrationDepth - Slop);

    if (biasPenetration > KINDA_SMALL) // 아주 작은 값은 무시
    {
        // Baumgarte 안정화 항: (위치 오류 * 보정 계수) / DeltaTime  
        // 위치 보정을 나타낼 속도 편향
        return (biasPenetration * BiasFactor) / DeltaTime;
    }
    return 0.0f;
}
