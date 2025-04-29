#include "CollisionDetector.h"
#include <algorithm>
#include "ConfigReadManager.h"
#include "DebugDrawerManager.h"

FCollisionDetector::FCollisionDetector()
{
	UConfigReadManager::Get()->GetValue("CCDTimeStep", CCDTimeStep);
}

FCollisionDetectionResult FCollisionDetector::DetectCollisionDiscrete(
	const ICollisionShape& ShapeA,
	const FTransform& TransformA,
	const ICollisionShape& ShapeB,
	const FTransform& TransformB)
{

	return DetectCollisionShapeBasedDiscrete(ShapeA, TransformA,
											 ShapeB, TransformB);
}

FCollisionDetectionResult FCollisionDetector::DetectCollisionCCD(
	const ICollisionShape& ShapeA,
	const FTransform& PrevTransformA,
	const FTransform& CurrentTransformA,
	const ICollisionShape& ShapeB,
	const FTransform& PrevTransformB,
	const FTransform& CurrentTransformB,
	const float DeltaTime)
{
	// 초기 상태와 최종 상태에서의 충돌 검사
	FCollisionDetectionResult startResult = DetectCollisionDiscrete(
		ShapeA, PrevTransformA, ShapeB, PrevTransformB);

	FCollisionDetectionResult endResult = DetectCollisionDiscrete(
		ShapeA, CurrentTransformA, ShapeB, CurrentTransformB);

	// 다음 프레임 트랜스폼 예측 (현재 속도 기반 외삽)
	// 위치는 등속 운동, 회전은 등각속도 운동으로 가정

	// 위치 예측: P_next = P_curr + (P_curr - P_prev)
	FTransform NextPredictedTransformA = CurrentTransformA;
	NextPredictedTransformA.Position = CurrentTransformA.Position +
		(CurrentTransformA.Position - PrevTransformA.Position);

	FTransform NextPredictedTransformB = CurrentTransformB;
	NextPredictedTransformB.Position = CurrentTransformB.Position +
		(CurrentTransformB.Position - PrevTransformB.Position);

	// 회전 예측: Q_next = Q_curr * (Q_prev^-1 * Q_curr)
	XMVECTOR prevQuatA = XMLoadFloat4(&PrevTransformA.Rotation);
	XMVECTOR currQuatA = XMLoadFloat4(&CurrentTransformA.Rotation);
	XMVECTOR prevQuatB = XMLoadFloat4(&PrevTransformB.Rotation);
	XMVECTOR currQuatB = XMLoadFloat4(&CurrentTransformB.Rotation);

	// 회전 변화량 계산
	XMVECTOR deltaQuatA = XMQuaternionMultiply(
		XMQuaternionInverse(prevQuatA), currQuatA);
	XMVECTOR deltaQuatB = XMQuaternionMultiply(
		XMQuaternionInverse(prevQuatB), currQuatB);

	// 다음 회전 예측
	XMVECTOR nextQuatA = XMQuaternionMultiply(currQuatA, deltaQuatA);
	XMVECTOR nextQuatB = XMQuaternionMultiply(currQuatB, deltaQuatB);

	// 정규화 및 저장
	nextQuatA = XMQuaternionNormalize(nextQuatA);
	nextQuatB = XMQuaternionNormalize(nextQuatB);

	XMStoreFloat4(&NextPredictedTransformA.Rotation, nextQuatA);
	XMStoreFloat4(&NextPredictedTransformB.Rotation, nextQuatB);

	// 스케일 예측: 단순 선형 외삽
	NextPredictedTransformA.Scale = CurrentTransformA.Scale +
		(CurrentTransformA.Scale - PrevTransformA.Scale);
	NextPredictedTransformB.Scale = CurrentTransformB.Scale +
		(CurrentTransformB.Scale - PrevTransformB.Scale);

	// 콘서버티브 어드밴스먼트를 사용한 충돌 시간 검색
	float currentTime = 0.0f;
	Vector3 collisionNormal = Vector3::Zero;
	FCollisionDetectionResult result = endResult; // 기본값으로 현재 프레임 상태 결과 사용
	result.bCollided = false;

	for (int iteration = 0; iteration < CCDMaxIterations && currentTime < DeltaTime; iteration++)
	{
		// 2차 보간으로 현재 시간에서의 트랜스폼 계산
		// t = currentTime / DeltaTime (0~1 범위)
		float t = currentTime / DeltaTime;

		// 베지어 보간 가중치
		float w0 = (1 - t) * (1 - t);   // 이전 프레임 가중치
		float w1 = 2 * t * (1 - t);     // 현재 프레임 가중치
		float w2 = t * t;           // 예측 프레임 가중치

		// 위치 보간
		FTransform interpA, interpB;

		// 위치 보간 (2차 베지어 곡선)
		interpA.Position = w0 * PrevTransformA.Position +
			w1 * CurrentTransformA.Position +
			w2 * NextPredictedTransformA.Position;

		interpB.Position = w0 * PrevTransformB.Position +
			w1 * CurrentTransformB.Position +
			w2 * NextPredictedTransformB.Position;

		// 회전 보간 (구면 선형 보간 사용)
		// 중간 프레임에서의 쿼터니언 보간
		XMVECTOR interpQuatA;
		if (t <= 0.5f)
		{
			// 이전과 현재 사이 보간
			float normalizedT = t * 2.0f; // 0~0.5 -> 0~1
			interpQuatA = XMQuaternionSlerp(prevQuatA, currQuatA, normalizedT);
		}
		else
		{
			// 현재와 예측 사이 보간
			float normalizedT = (t - 0.5f) * 2.0f; // 0.5~1 -> 0~1
			interpQuatA = XMQuaternionSlerp(currQuatA, nextQuatA, normalizedT);
		}

		XMVECTOR interpQuatB;
		if (t <= 0.5f)
		{
			float normalizedT = t * 2.0f;
			interpQuatB = XMQuaternionSlerp(prevQuatB, currQuatB, normalizedT);
		}
		else
		{
			float normalizedT = (t - 0.5f) * 2.0f;
			interpQuatB = XMQuaternionSlerp(currQuatB, nextQuatB, normalizedT);
		}

		XMStoreFloat4(&interpA.Rotation, XMQuaternionNormalize(interpQuatA));
		XMStoreFloat4(&interpB.Rotation, XMQuaternionNormalize(interpQuatB));

		// 스케일 보간 (선형 보간)
		interpA.Scale = w0 * PrevTransformA.Scale +
			w1 * CurrentTransformA.Scale +
			w2 * NextPredictedTransformA.Scale;

		interpB.Scale = w0 * PrevTransformB.Scale +
			w1 * CurrentTransformB.Scale +
			w2 * NextPredictedTransformB.Scale;

		// 충돌 검사
		FCollisionDetectionResult tempResult = DetectCollisionDiscrete(
			ShapeA, interpA, ShapeB, interpB);

		// 충돌 발생한 경우
		if (tempResult.bCollided)
		{
			tempResult.TimeOfImpact = currentTime;
			LOG("%d CCD detected", currentTime);
			return tempResult;
		}

		// 최소 거리 계산
		float separation = ComputeMinimumDistance(ShapeA, interpA, ShapeB, interpB);

		// 이미 충돌 상태라면 바로 반환
		if (separation <= DistanceThreshold)
		{
			tempResult.bCollided = true;
			tempResult.TimeOfImpact = currentTime;
			return tempResult;
		}

		// 충돌 법선 계산 (B에서 A 방향)
		collisionNormal = interpB.Position - interpA.Position;
		if (collisionNormal.LengthSquared() > KINDA_SMALL)
		{
			collisionNormal.Normalize();
		}
		else
		{
			collisionNormal = Vector3::Up; // 기본 방향
		}

		// 현재 시간에서의 상대 속도 계산
		// 미분을 통한 순간 속도 계산 (2차 베지어 곡선의 미분)
		Vector3 velocityA = (1 - t) * 2 * (CurrentTransformA.Position - PrevTransformA.Position) +
			t * 2 * (NextPredictedTransformA.Position - CurrentTransformA.Position);

		Vector3 velocityB = (1 - t) * 2 * (CurrentTransformB.Position - PrevTransformB.Position) +
			t * 2 * (NextPredictedTransformB.Position - CurrentTransformB.Position);

		// 순간 속도를 초당 속도로 변환
		velocityA = velocityA / DeltaTime;
		velocityB = velocityB / DeltaTime;

		// 접촉점 추정
		Vector3 contactPoint = interpA.Position +
			ShapeA.GetSupportPoint(collisionNormal);

		// 회전 속도 추정 및 접촉점에서의 속도 계산
		// (간소화를 위해 선형 속도만 고려)
		Vector3 relativeVelocity = velocityB - velocityA;

		// 접근 속도 계산
		float approachSpeed = ComputeApproachSpeed(relativeVelocity, collisionNormal);

		// 접근 속도가 매우 작으면 안전하게 끝까지 이동
		if (approachSpeed < KINDA_SMALL)
		{
			// 안전한 큰 시간 스텝 사용
			currentTime += (DeltaTime - currentTime) * 0.5f;
			continue;
		}

		// 안전한 이동 시간 계산 (콘서버티브 어드밴스먼트 핵심 수식)
		float safeTimeStep = separation / (approachSpeed * CASafetyFactor);

		// 최소 시간 스텝 보장
		safeTimeStep = std::max(CAMinTimeStep, safeTimeStep);

		// 남은 시간을 초과하지 않도록 제한
		float remainingTime = DeltaTime - currentTime;
		safeTimeStep = std::min(safeTimeStep, remainingTime);

		// 시간 진행
		currentTime += safeTimeStep;

		// 수렴 확인 (진전이 매우 작으면 종료)
		if (safeTimeStep < CAMinTimeStep || safeTimeStep < remainingTime * 0.001f)
		{
			LOG("CA Skipped");
			break;
		}
	}

	// 최대 반복 횟수 도달 후에도 충돌이 없으면 최종 상태 결과 반환
	LOG("CA Found No COllision");
	return endResult;
}

FCollisionDetectionResult FCollisionDetector::DetectCollisionShapeBasedDiscrete(const ICollisionShape& ShapeA, const FTransform& TransformA, const ICollisionShape& ShapeB, const FTransform& TransformB)
{
	// 형상 타입에 따른 적절한 충돌 검사 함수 호출
	if (ShapeA.GetType() == ECollisionShapeType::Sphere && ShapeB.GetType() == ECollisionShapeType::Sphere)
	{
		return SphereSphere(
			ShapeA.GetScaledHalfExtent().x, TransformA,
			ShapeB.GetScaledHalfExtent().x, TransformB);
	}
	else if (ShapeA.GetType() == ECollisionShapeType::Box && ShapeB.GetType() == ECollisionShapeType::Box)
	{
		return BoxBoxSAT(
			ShapeA.GetScaledHalfExtent(), TransformA,
			ShapeB.GetScaledHalfExtent(), TransformB);
	}
	else if (ShapeA.GetType() == ECollisionShapeType::Box && ShapeB.GetType() == ECollisionShapeType::Sphere)
	{
		return BoxSphereSimple(
			ShapeA.GetScaledHalfExtent(), TransformA,
			ShapeB.GetScaledHalfExtent().x, TransformB);
	}
	else if (ShapeA.GetType() == ECollisionShapeType::Sphere && ShapeB.GetType() == ECollisionShapeType::Box)
	{
		auto Result = BoxSphereSimple(
			ShapeB.GetScaledHalfExtent(), TransformB,
			ShapeA.GetScaledHalfExtent().x, TransformA);
		// 노멀 방향 반전
		Result.Normal = -Result.Normal;
		return Result;
	}
	return FCollisionDetectionResult();
}

#pragma region Shape-Based
FCollisionDetectionResult FCollisionDetector::SphereSphere(
	float RadiusA, const FTransform& TransformA,
	float RadiusB, const FTransform& TransformB)
{
	FCollisionDetectionResult Result;

	// SIMD 연산을 위한 벡터 로드
	XMVECTOR vPosA = XMLoadFloat3(&TransformA.Position);
	XMVECTOR vPosB = XMLoadFloat3(&TransformB.Position);

	// 중심점 간의 벡터 계산
	XMVECTOR vDelta = XMVectorSubtract(vPosB, vPosA);
	float distanceSquared = XMVectorGetX(XMVector3LengthSq(vDelta));

	float radiusSum = RadiusA + RadiusB;
	float penetrationDepth = radiusSum - sqrt(distanceSquared);

	// 충돌 검사
	if (penetrationDepth <= KINDA_SMALLER)
	{
		return Result;  // 기본값 반환 (충돌 없음)
	}

	Result.bCollided = true;

	// 충돌 정보 설정
	float distance = sqrt(distanceSquared);
	if (distance < KINDA_SMALL)
	{
		// 구체가 거의 같은 위치에 있는 경우
		Result.Normal = Vector3::Up;  // 기본 방향 설정
		Result.PenetrationDepth = radiusSum;
		XMStoreFloat3(&Result.Point, vPosA);
	}
	else
	{
		// 정규화된 충돌 법선 계산
		XMVECTOR vNormal = XMVectorDivide(vDelta, XMVectorReplicate(distance));
		XMStoreFloat3(&Result.Normal, vNormal);
		Result.PenetrationDepth = penetrationDepth;

		// 충돌 지점은 두 구의 중심을 잇는 선상에서 첫 번째 구의 표면에 위치
		XMVECTOR vCollisionPoint = XMVectorAdd(vPosA,
											   XMVectorMultiply(vNormal, XMVectorReplicate(RadiusA)));
		XMStoreFloat3(&Result.Point, vCollisionPoint);
	}

	return Result;
}

FCollisionDetectionResult FCollisionDetector::BoxBoxAABB(
	const Vector3& ExtentA, const FTransform& TransformA,
	const Vector3& ExtentB, const FTransform& TransformB)
{
	FCollisionDetectionResult Result;

	// 월드 공간에서의 박스 최소/최대점 계산
	Vector3 MinA = TransformA.Position - ExtentA;
	Vector3 MaxA = TransformA.Position + ExtentA;
	Vector3 MinB = TransformB.Position - ExtentB;
	Vector3 MaxB = TransformB.Position + ExtentB;

	// AABB 충돌 검사
	if (MinA.x <= MaxB.x && MaxA.x >= MinB.x &&
		MinA.y <= MaxB.y && MaxA.y >= MinB.y &&
		MinA.z <= MaxB.z && MaxA.z >= MinB.z)
	{
		Result.bCollided = true;

		// 각 축의 겹침(침투) 정도 계산
		Vector3 PenetrationVec;
		PenetrationVec.x = std::min(MaxA.x - MinB.x, MaxB.x - MinA.x);
		PenetrationVec.y = std::min(MaxA.y - MinB.y, MaxB.y - MinA.y);
		PenetrationVec.z = std::min(MaxA.z - MinB.z, MaxB.z - MinA.z);

		// 가장 얕은 침투 방향을 충돌 법선으로 사용
		int minIndex = 0;
		Result.PenetrationDepth = PenetrationVec.x;
		if (PenetrationVec.y < Result.PenetrationDepth)
		{
			minIndex = 1;
			Result.PenetrationDepth = PenetrationVec.y;
		}
		if (PenetrationVec.z < Result.PenetrationDepth)
		{
			minIndex = 2;
			Result.PenetrationDepth = PenetrationVec.z;
		}

		// 충돌 법선 설정
		switch (minIndex)
		{
			case 0: Result.Normal.x = (TransformA.Position.x < TransformB.Position.x) ? -1.0f : 1.0f; break;
			case 1: Result.Normal.y = (TransformA.Position.y < TransformB.Position.y) ? -1.0f : 1.0f; break;
			case 2: Result.Normal.z = (TransformA.Position.z < TransformB.Position.z) ? -1.0f : 1.0f; break;
		}

		// 충돌 지점은 두 박스의 중심점 사이의 중간점으로 설정
		Result.Point = (TransformA.Position + TransformB.Position) * 0.5f;
	}

	return Result;
}

FCollisionDetectionResult FCollisionDetector::BoxBoxSAT(
	const Vector3& ExtentA, const FTransform& TransformA,
	const Vector3& ExtentB, const FTransform& TransformB)
{
	FCollisionDetectionResult Result;

	Matrix RotationA = TransformA.GetRotationMatrix();
	Matrix RotationB = TransformB.GetRotationMatrix();

	XMVECTOR vPosA = XMLoadFloat3(&TransformA.Position);
	XMVECTOR vPosB = XMLoadFloat3(&TransformB.Position);
	XMVECTOR vDelta = XMVectorSubtract(vPosB, vPosA);

	// 각 박스의 축 방향 벡터 추출
	XMVECTOR vAxesA[3] = {
		RotationA.r[0],
		RotationA.r[1],
		RotationA.r[2]
	};

	XMVECTOR vAxesB[3] = {
		RotationB.r[0],
		RotationB.r[1],
		RotationB.r[2]
	};

	// SAT 검사를 위한 축 목록
	XMVECTOR vSeparatingAxes[15];
	int axisIndex = 0;

	// 박스 A의 축
	for (int i = 0; i < 3; i++)
		vSeparatingAxes[axisIndex++] = vAxesA[i];

	// 박스 B의 축
	for (int i = 0; i < 3; i++)
		vSeparatingAxes[axisIndex++] = vAxesB[i];

	// 축 간의 외적
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			vSeparatingAxes[axisIndex++] = XMVector3Cross(vAxesA[i], vAxesB[j]);
		}
	}

	float minPenetration = FLT_MAX;
	XMVECTOR vCollisionNormal = XMVectorZero();

	// 각 축에 대해 투영 검사
	for (int i = 0; i < 15; i++)
	{
		XMVECTOR vLengthSq = XMVector3LengthSq(vSeparatingAxes[i]);
		if (XMVectorGetX(vLengthSq) < KINDA_SMALL)
			continue;

		XMVECTOR vAxis = XMVector3Normalize(vSeparatingAxes[i]);

		// 각 박스의 투영 반경 계산
		float radiusA =
			abs(XMVectorGetX(XMVector3Dot(vAxis, XMVectorScale(vAxesA[0], ExtentA.x)))) +
			abs(XMVectorGetX(XMVector3Dot(vAxis, XMVectorScale(vAxesA[1], ExtentA.y)))) +
			abs(XMVectorGetX(XMVector3Dot(vAxis, XMVectorScale(vAxesA[2], ExtentA.z))));

		float radiusB =
			abs(XMVectorGetX(XMVector3Dot(vAxis, XMVectorScale(vAxesB[0], ExtentB.x)))) +
			abs(XMVectorGetX(XMVector3Dot(vAxis, XMVectorScale(vAxesB[1], ExtentB.y)))) +
			abs(XMVectorGetX(XMVector3Dot(vAxis, XMVectorScale(vAxesB[2], ExtentB.z))));

		float distance = abs(XMVectorGetX(XMVector3Dot(vDelta, vAxis)));
		float penetration = radiusA + radiusB - distance;

		if (penetration <= 0)
			return Result;  // 분리축 발견

		if (penetration < minPenetration)
		{
			minPenetration = penetration;
			float direction = XMVectorGetX(XMVector3Dot(vDelta, vAxis));
			vCollisionNormal = direction >= 0 ? vAxis : XMVectorNegate(vAxis);
		}
	}

	// 충돌 발생
	Result.bCollided = true;
	XMStoreFloat3(&Result.Normal, vCollisionNormal);
	Result.PenetrationDepth = minPenetration;

	// 충돌 지점 계산
	XMVECTOR vCollisionPoint = XMVectorAdd(vPosA,
										   XMVectorMultiply(vCollisionNormal, XMVectorReplicate(minPenetration * 0.5f)));
	XMStoreFloat3(&Result.Point, vCollisionPoint);

	return Result;
}

FCollisionDetectionResult FCollisionDetector::BoxSphereSimple(
	const Vector3& BoxExtent, const FTransform& BoxTransform,
	float SphereRadius, const FTransform& SphereTransform)
{
	FCollisionDetectionResult Result;

	// 1. 박스의 로컬 공간으로 구공간 이동
	// 박스의 회전을 고려하기 위해 역회전 행렬 사용
	XMVECTOR vSpherePos = XMLoadFloat3(&SphereTransform.Position);
	XMVECTOR vBoxPos = XMLoadFloat3(&BoxTransform.Position);
	Matrix BoxRotationInv = XMMatrixTranspose(BoxTransform.GetRotationMatrix());

	// 구의 중심점 회전
	XMVECTOR vRelativePos = XMVectorSubtract(vSpherePos, vBoxPos);  // 상대 위치
	XMVECTOR vLocalSpherePos = XMVector3Transform(vRelativePos, BoxRotationInv);

	// 2. 박스의 로컬 공간에서 구의 중심점에 가장 가까운 점 찾기
	Vector3 localSphere;
	XMStoreFloat3(&localSphere, vLocalSpherePos);

	Vector3 closestPoint;
	// 각 축별로 박스의 범위로 클램핑
	closestPoint.x = Math::Clamp(localSphere.x, -BoxExtent.x, BoxExtent.x);
	closestPoint.y = Math::Clamp(localSphere.y, -BoxExtent.y, BoxExtent.y);
	closestPoint.z = Math::Clamp(localSphere.z, -BoxExtent.z, BoxExtent.z);

	// 3. 구의 중심점과 가장 가까운 점 사이의 거리 계산
	XMVECTOR vClosestPoint = XMLoadFloat3(&closestPoint);
	XMVECTOR vDelta = XMVectorSubtract(vLocalSpherePos, vClosestPoint);
	float distanceSquared = XMVectorGetX(XMVector3LengthSq(vDelta));

	// 충돌 검사: 거리가 구의 반지름보다 크면 충돌하지 않음
	if (distanceSquared > SphereRadius * SphereRadius)
		return Result;  // 충돌 없음

	// 4. 충돌 정보 계산
	Result.bCollided = true;
	float distance = sqrt(distanceSquared);

	// 구의 중심이 박스와 거의 일치하는 경우
	if (distance < KINDA_SMALL)
	{
		// 최소 침투 방향 찾기
		float penetrations[6] = {
			BoxExtent.x + localSphere.x,  // -x 방향
			BoxExtent.x - localSphere.x,  // +x 방향
			BoxExtent.y + localSphere.y,  // -y 방향
			BoxExtent.y - localSphere.y,  // +y 방향
			BoxExtent.z + localSphere.z,  // -z 방향
			BoxExtent.z - localSphere.z   // +z 방향
		};

		// 가장 얕은 침투 방향 찾기
		int minIndex = 0;
		float minPenetration = penetrations[0];
		for (int i = 1; i < 6; i++)
		{
			if (penetrations[i] < minPenetration)
			{
				minPenetration = penetrations[i];
				minIndex = i;
			}
		}

		// 최소 침투 방향으로 법선 설정
		Vector3 localNormal = Vector3::Zero;
		switch (minIndex)
		{
			case 0: localNormal.x = -1.0f; break;  // -x 방향
			case 1: localNormal.x = 1.0f; break;   // +x 방향
			case 2: localNormal.y = -1.0f; break;  // -y 방향
			case 3: localNormal.y = 1.0f; break;   // +y 방향
			case 4: localNormal.z = -1.0f; break;  // -z 방향
			case 5: localNormal.z = 1.0f; break;   // +z 방향
		}

		XMVECTOR vNormal = XMLoadFloat3(&localNormal);
		Result.PenetrationDepth = minPenetration + SphereRadius;

		// 법선을 월드 공간으로 변환
		Matrix BoxRotation = BoxTransform.GetRotationMatrix();
		vNormal = XMVector3Transform(vNormal, BoxRotation);
		XMStoreFloat3(&Result.Normal, vNormal);

		// 충돌 지점은 박스의 표면
		XMVECTOR vWorldClosestPoint = XMVector3Transform(vClosestPoint, BoxRotation);
		vWorldClosestPoint = XMVectorAdd(vWorldClosestPoint, vBoxPos);
		XMStoreFloat3(&Result.Point, vWorldClosestPoint);
	}
	// 구가 박스 외부에 있는 일반적인 경우
	else
	{
		// 로컬 공간에서의 방향 벡터를 법선으로 사용
		XMVECTOR vNormal = XMVector3Normalize(vDelta);
		Result.PenetrationDepth = SphereRadius - distance;

		// 법선을 월드 공간으로 변환
		Matrix BoxRotation = BoxTransform.GetRotationMatrix();
		vNormal = XMVector3Transform(vNormal, BoxRotation);
		XMStoreFloat3(&Result.Normal, vNormal);

		// 충돌 지점은 구의 표면에서 법선 방향으로의 지점
		XMVECTOR vWorldClosestPoint = XMVector3Transform(vClosestPoint, BoxRotation);
		vWorldClosestPoint = XMVectorAdd(vWorldClosestPoint, vBoxPos);
		XMStoreFloat3(&Result.Point, vWorldClosestPoint);
	}

	return Result;
}
#pragma endregion
//////////////////////////////////////
#pragma region Conservative Advanced
float FCollisionDetector::ComputeMinimumDistance(
	const ICollisionShape& ShapeA,
	const FTransform& TransformA,
	const ICollisionShape& ShapeB,
	const FTransform& TransformB) const
{
	// TTODO : GJK 알고리즘을 사용하여 두 충돌체 간의 최소 거리 계산
	// 이 구현은 간소화된 버전으로, 실제로는 더 정교한 GJK 거리 계산 알고리즘 필요

	// 각 도형의 지원점을 사용해 최소 거리 추정
	// 먼저 대략적인 분리 방향 설정 (B에서 A 방향)
	Vector3 direction = TransformB.Position - TransformA.Position;

	if (direction.LengthSquared() < KINDA_SMALL)
	{
		// 중심이 거의 같은 경우 임의 방향 설정
		direction = Vector3::Up;
	}
	else
	{
		direction.Normalize();
	}

	// GJK 유사 접근법으로 거리 계산 반복
	for (int i = 0; i < MAX_GJK_ITERATIONS; i++)
	{
		// 각 충돌체의 지원점 계산
		Vector3 supportA = ShapeA.GetSupportPoint(-direction);
		Vector3 supportB = ShapeB.GetSupportPoint(direction);

		// 지원점 간의 벡터
		Vector3 minkowskiPoint = supportB - supportA;

		// 새 방향 계산
		float distSq = minkowskiPoint.LengthSquared();

		if (distSq < KINDA_SMALL)
		{
			// 충돌 상태 (거리가 0)
			return 0.0f;
		}

		// 새로운 방향 계산
		Vector3 newDirection = minkowskiPoint;
		newDirection.Normalize();

		// 방향이 크게 변경되지 않았다면 수렴으로 간주
		if (Vector3::Dot(newDirection, direction) > 0.99f)
		{
			return sqrt(distSq);
		}

		direction = newDirection;
	}

	// 최대 반복 후 현재 거리 반환
	// 마지막 지원점 간의 거리 계산
	Vector3 supportA = ShapeA.GetSupportPoint(-direction);
	Vector3 supportB = ShapeB.GetSupportPoint(direction);
	Vector3 minkowskiPoint = supportB - supportA;

	return minkowskiPoint.Length();
}


Vector3 FCollisionDetector::ComputeRelativeVelocity(
	const FTransform& PrevTransformA,
	const FTransform& CurrentTransformA,
	const FTransform& PrevTransformB,
	const FTransform& CurrentTransformB,
	const Vector3& ContactPoint,
	float DeltaTime) const
{
	// 선형 속도 계산
	Vector3 linearVelocityA = (CurrentTransformA.Position - PrevTransformA.Position) / DeltaTime;
	Vector3 linearVelocityB = (CurrentTransformB.Position - PrevTransformB.Position) / DeltaTime;

	// 각속도 계산 (쿼터니언 시간 변화율)
	XMVECTOR prevQuatA = XMLoadFloat4(&PrevTransformA.Rotation);
	XMVECTOR currQuatA = XMLoadFloat4(&CurrentTransformA.Rotation);
	XMVECTOR prevQuatB = XMLoadFloat4(&PrevTransformB.Rotation);
	XMVECTOR currQuatB = XMLoadFloat4(&CurrentTransformB.Rotation);

	// 각속도를 쿼터니언 변화로 추정
	XMVECTOR quatDiffA = XMQuaternionMultiply(
		currQuatA, XMQuaternionInverse(prevQuatA));
	XMVECTOR quatDiffB = XMQuaternionMultiply(
		currQuatB, XMQuaternionInverse(prevQuatB));

	// 쿼터니언을 Angle-Axis로 변환
	float angleA, angleB;
	XMVECTOR axisA, axisB;

	// A 물체의 쿼터니언에서 회전 축과 각도 추출
	XMVECTOR quatDiffWXYZA = XMVectorSwizzle(quatDiffA, 3, 0, 1, 2); // w를 앞으로
	angleA = 2.0f * acos(XMVectorGetX(quatDiffWXYZA)); // w 성분으로 각도 추출

	if (angleA > KINDA_SMALL)
	{
		float sinHalfAngleInvA = 1.0f / sin(angleA * 0.5f);
		axisA = XMVectorScale(
			XMVectorSwizzle(quatDiffWXYZA, 1, 2, 3, 0), // xyz 성분
			sinHalfAngleInvA);
	}
	else
	{
		axisA = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f); // 회전이 미미하면 기본 축 사용(y)
		angleA = 0.0f;
	}

	// B 물체의 쿼터니언에서 회전 축과 각도 추출
	XMVECTOR quatDiffWXYZB = XMVectorSwizzle(quatDiffB, 3, 0, 1, 2);
	angleB = 2.0f * acos(XMVectorGetX(quatDiffWXYZB));

	if (angleB > KINDA_SMALL)
	{
		float sinHalfAngleInvB = 1.0f / sin(angleB * 0.5f);
		axisB = XMVectorScale(
			XMVectorSwizzle(quatDiffWXYZB, 1, 2, 3, 0),
			sinHalfAngleInvB);
	}
	else
	{
		axisB = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
		angleB = 0.0f;
	}

	// 각속도 계산
	Vector3 angularVelocityA, angularVelocityB;
	XMStoreFloat3(&angularVelocityA, XMVectorScale(axisA, angleA / DeltaTime));
	XMStoreFloat3(&angularVelocityB, XMVectorScale(axisB, angleB / DeltaTime));

	// 접촉점 상대 위치
	Vector3 rA = ContactPoint - CurrentTransformA.Position;
	Vector3 rB = ContactPoint - CurrentTransformB.Position;

	// 회전으로 인한 선형 속도 계산
	Vector3 angularLinearA = Vector3::Cross(angularVelocityA, rA);
	Vector3 angularLinearB = Vector3::Cross(angularVelocityB, rB);

	// 접촉점에서의 총 상대 속도
	Vector3 totalVelocityA = linearVelocityA + angularLinearA;
	Vector3 totalVelocityB = linearVelocityB + angularLinearB;

	// B에서 A를 뺀 상대 속도
	return totalVelocityB - totalVelocityA;
}

float FCollisionDetector::ComputeApproachSpeed(
	const Vector3& RelativeVelocity,
	const Vector3& Direction) const
{
	// 상대 속도를 방향에 투영하여 접근 속도 계산
	// 양수이면 서로 멀어지는 속도, 음수이면 접근 속도
	float projectedSpeed = Vector3::Dot(RelativeVelocity, Direction);

	// 접근 속도를 양수로 반환 (멀어지는 경우 0 반환)
	return std::max(0.0f, -projectedSpeed);
}

float FCollisionDetector::PerformConservativeAdvancementStep(
	const ICollisionShape& ShapeA,
	const FTransform& PrevTransformA,
	const FTransform& CurrentTransformA,
	const ICollisionShape& ShapeB,
	const FTransform& PrevTransformB,
	const FTransform& CurrentTransformB,
	float CurrentTime,
	float DeltaTime,
	Vector3& CollisionNormal) const
{
	// 현재 시간에서의 보간된 트랜스폼 계산
	FTransform interpA = FTransform::InterpolateTransform(PrevTransformA, CurrentTransformA, CurrentTime / DeltaTime);
	FTransform interpB = FTransform::InterpolateTransform(PrevTransformB, CurrentTransformB, CurrentTime / DeltaTime);

	// 최소 거리 계산
	float separation = ComputeMinimumDistance(ShapeA, interpA, ShapeB, interpB);

	// 이미 충돌 상태라면 바로 반환
	if (separation <= DistanceThreshold)
	{
		return 0.0f;
	}

	// 충돌 법선 계산 (B에서 A 방향)
	CollisionNormal = interpB.Position - interpA.Position;
	if (CollisionNormal.LengthSquared() > KINDA_SMALL)
	{
		CollisionNormal.Normalize();
	}
	else
	{
		CollisionNormal = Vector3::Up; // 기본 방향
	}

	// 접촉점 추정 (실제 구현에서는 더 정확한 계산 필요)
	Vector3 contactPoint = interpA.Position +
		ShapeA.GetSupportPoint(CollisionNormal);

	// 상대 속도 계산
	Vector3 relativeVelocity = ComputeRelativeVelocity(
		PrevTransformA, CurrentTransformA,
		PrevTransformB, CurrentTransformB,
		contactPoint, DeltaTime);

	// 접근 속도 계산
	float approachSpeed = ComputeApproachSpeed(relativeVelocity, CollisionNormal);

	// 접근 속도가 매우 작으면 안전하게 끝까지 이동
	if (approachSpeed < KINDA_SMALL)
	{
		return DeltaTime - CurrentTime; // 남은 시간 전체
	}

	// 안전한 이동 시간 계산 (콘서버티브 어드밴스먼트 핵심 수식)
	float safeTimeStep = separation / (approachSpeed * CASafetyFactor);

	// 최소 시간 스텝 보장
	safeTimeStep = std::max(CAMinTimeStep, safeTimeStep);

	// 남은 시간을 초과하지 않도록 제한
	return std::min(safeTimeStep, DeltaTime - CurrentTime);
}
#pragma endregion