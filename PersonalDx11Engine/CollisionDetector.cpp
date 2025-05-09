#include "CollisionDetector.h"
#include <algorithm>
#include "ConfigReadManager.h"
#include "DebugDrawerManager.h"
#include <unordered_set>

FCollisionDetector::FCollisionDetector()
{
	UConfigReadManager::Get()->GetValue("CCDTimeStep", CCDTimeStep);
	UConfigReadManager::Get()->GetValue("bUseGJKEPA", bUseGJKEPA);
	UConfigReadManager::Get()->GetValue("CASafetyFactor", CASafetyFactor);
	UConfigReadManager::Get()->GetValue("MaxGJKIterations", MaxGJKIterations);
	UConfigReadManager::Get()->GetValue("MaxEPAIterations", MaxEPAIterations);
	UConfigReadManager::Get()->GetValue("EPATolerance", EPATolerance);
}

FCollisionDetectionResult FCollisionDetector::DetectCollisionDiscrete(
	const ICollisionShape& ShapeA,
	const FTransform& TransformA,
	const ICollisionShape& ShapeB,
	const FTransform& TransformB)
{

	FCollisionDetectionResult Result; 
	if (bUseGJKEPA)
	{
		Result =  DetectCollisionGJKEPA(ShapeA, TransformA,
									 ShapeB, TransformB);
	}
	else
	{
		Result = DetectCollisionShapeBasedDiscrete(ShapeA, TransformA,
												   ShapeB, TransformB);
	}

	if (Result.bCollided)
	{
		auto sNormal = Debug::ToString(Result.Normal, "");
		LOG("[PDepth] : %.3f \n[ToImpace] : %.3f", Result.PenetrationDepth, Result.TimeOfImpact);
		LOG("[DetectNormal] : %s", sNormal);
	}
	return Result;
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
	//test dcd
	if (bUseGJKEPA)
	{
		return DetectCollisionGJKEPA(ShapeA, CurrentTransformA,
									 ShapeB, CurrentTransformB);
	}

	return DetectCollisionShapeBasedDiscrete(ShapeA, CurrentTransformA,
											 ShapeB, CurrentTransformB);
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

#pragma region GJK_EPA
FCollisionDetectionResult FCollisionDetector::DetectCollisionGJKEPA(
	const ICollisionShape& ShapeA,
	const FTransform& TransformA,
	const ICollisionShape& ShapeB,
	const FTransform& TransformB)
{
	// 결과 구조체 초기화
	FCollisionDetectionResult Result;

	// Simplex 구조체 생성
	FSimplex Simplex;
	Simplex.Size = 0;

	// GJK로 충돌 여부 확인
	if (!GJKCollision(ShapeA, TransformA, ShapeB, TransformB, Simplex))
	{
		// 충돌 없음
		return Result;
	}
	else
	{
		LOG("--------[DetectGJK]------");
	}

	// EPA로 충돌 정보 계산
	Result = EPACollision(ShapeA, TransformA, ShapeB, TransformB, Simplex);
	return Result;
}

bool FCollisionDetector::GJKCollision(
	const ICollisionShape& ShapeA,
	const FTransform& TransformA,
	const ICollisionShape& ShapeB,
	const FTransform& TransformB,
	FSimplex& OutSimplex)
{
	// 초기 방향 설정 (B에서 A 방향)
	XMVECTOR Direction = XMVectorSubtract(
		XMLoadFloat3(&TransformB.Position),
		XMLoadFloat3(&TransformA.Position));
	if (XMVector3LengthSq(Direction).m128_f32[0] < KINDA_SMALL)
	{
		Direction = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f); // 기본 방향
	}
	Direction = XMVector3Normalize(Direction);

	// 초기 Simplex 설정
	OutSimplex.Size = 0;

	// 초기 지원점 추가
	XMVECTOR SupportA, SupportB;
	XMVECTOR Point = ComputeMinkowskiSupport(
		ShapeA, TransformA, ShapeB, TransformB, Direction, SupportA, SupportB);
	OutSimplex.Points[0] = Point;
	OutSimplex.SupportPointsA[0] = SupportA;
	OutSimplex.SupportPointsB[0] = SupportB;
	OutSimplex.Size = 1;

	// 원점 방향으로 방향 반전
	Direction = XMVector3Normalize( XMVectorNegate(Point));

	// GJK 반복
	for (int Iteration = 0; Iteration < MaxGJKIterations; ++Iteration)
	{
		// 새로운 지원점 계산
		Point = ComputeMinkowskiSupport(
			ShapeA, TransformA, ShapeB, TransformB, Direction, SupportA, SupportB);

		// 원점을 지나지 못하면 충돌 없음
		if (XMVector3Dot(Point, Direction).m128_f32[0] < 0)
		{
			return false;
		}

		// Simplex에 점 추가
		OutSimplex.Points[OutSimplex.Size] = Point;
		OutSimplex.SupportPointsA[OutSimplex.Size] = SupportA;
		OutSimplex.SupportPointsB[OutSimplex.Size] = SupportB;
		++OutSimplex.Size;

		// Simplex 업데이트 및 원점 포함 여부 확인
		if (UpdateSimplex(OutSimplex, Direction))
		{
			return true; // 원점 포함, 충돌 발생
		}
	}

	// 최대 반복 횟수 초과
	return true;
}

XMVECTOR FCollisionDetector::ComputeMinkowskiSupport(
	const ICollisionShape& ShapeA,
	const FTransform& TransformA,
	const ICollisionShape& ShapeB,
	const FTransform& TransformB,
	const XMVECTOR& Direction,
	XMVECTOR& OutSupportA,
	XMVECTOR& OutSupportB)
{
	// ShapeA의 지원점 계산 (반대 방향)
	XMVECTOR NegDirection = XMVectorNegate(Direction);

	Vector3 NegDir = Vector3(NegDirection.m128_f32[0], NegDirection.m128_f32[1], NegDirection.m128_f32[2]);
	Vector3 Dir = Vector3(Direction.m128_f32[0], Direction.m128_f32[1], Direction.m128_f32[2]);
	
	//ShapeA의 지원점 게산
	Vector3 SupportA = ShapeA.GetWorldSupportPoint(NegDir);
	OutSupportA = XMLoadFloat3(&SupportA);

	// ShapeB의 지원점 계산
	Vector3 SupportB = ShapeB.GetWorldSupportPoint(Dir);
	OutSupportB = XMLoadFloat3(&SupportB);

	// Minkowski 차분 계산
	return XMVectorSubtract(OutSupportB, OutSupportA);
}

bool FCollisionDetector::UpdateSimplex(FSimplex& Simplex, XMVECTOR& Direction)
{
	LOG("PreUpdate simplex %d", Simplex.Size);
	// Simplex 크기에 따른 처리
	switch (Simplex.Size)
	{
		case 2: // 선분
		{
			// 선분 AB와 원점
			XMVECTOR A = Simplex.Points[0];
			XMVECTOR B = Simplex.Points[1];
			XMVECTOR AB = XMVectorSubtract(B, A);
			XMVECTOR AO = XMVectorNegate(A);

			// 원점이 선분에 가장 가까운지 확인
			float DotAB_AO = XMVector3Dot(AB, AO).m128_f32[0];
			float DotAB_AB = XMVector3Dot(AB, AB).m128_f32[0];

			if (DotAB_AO <= 0.0f)
			{
				// A 방향
				Simplex.Points[0] = A;
				Simplex.SupportPointsA[0] = Simplex.SupportPointsA[0];
				Simplex.SupportPointsB[0] = Simplex.SupportPointsB[0];
				Simplex.Size = 1;
				Direction = AO;
			}
			else if (DotAB_AO >= DotAB_AB)
			{
				// B 방향
				Simplex.Points[0] = B;
				Simplex.SupportPointsA[0] = Simplex.SupportPointsA[1];
				Simplex.SupportPointsB[0] = Simplex.SupportPointsB[1];
				Simplex.Size = 1;
				Direction = XMVectorNegate(B);
			}
			else
			{
				// 선분에 수직인 방향
				Direction = XMVector3Cross(XMVector3Cross(AB, AO), AB);
				if (XMVector3LengthSq(Direction).m128_f32[0] < KINDA_SMALL)
				{
					Direction = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
				}
				Direction = XMVector3Normalize(Direction);
			}
			return false;
		}

		case 3: // 삼각형
		{
			// 삼각형 ABC와 원점
			XMVECTOR A = Simplex.Points[0];
			XMVECTOR B = Simplex.Points[1];
			XMVECTOR C = Simplex.Points[2];
			XMVECTOR AO = XMVectorNegate(A);
			XMVECTOR AB = XMVectorSubtract(B, A);
			XMVECTOR AC = XMVectorSubtract(C, A);
			XMVECTOR ABC = XMVector3Cross(AB, AC);

			// 법선 방향 확인
			if (XMVector3Dot(XMVector3Cross(ABC, AC), AO).m128_f32[0] > 0.0f)
			{
				// AC 방향
				Simplex.Points[0] = A;
				Simplex.Points[1] = C;
				Simplex.SupportPointsA[0] = Simplex.SupportPointsA[0];
				Simplex.SupportPointsA[1] = Simplex.SupportPointsA[2];
				Simplex.SupportPointsB[0] = Simplex.SupportPointsB[0];
				Simplex.SupportPointsB[1] = Simplex.SupportPointsB[2];
				Simplex.Size = 2;
				Direction = XMVector3Cross(XMVector3Cross(AC, AO), AC);
				if (XMVector3LengthSq(Direction).m128_f32[0] < KINDA_SMALL)
				{
					Direction = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
				}
				return false;
			}

			if (XMVector3Dot(XMVector3Cross(AB, ABC), AO).m128_f32[0] > 0.0f)
			{
				// AB 방향
				Simplex.Points[0] = A;
				Simplex.Points[1] = B;
				Simplex.SupportPointsA[0] = Simplex.SupportPointsA[0];
				Simplex.SupportPointsA[1] = Simplex.SupportPointsA[1];
				Simplex.SupportPointsB[0] = Simplex.SupportPointsB[0];
				Simplex.SupportPointsB[1] = Simplex.SupportPointsB[1];
				Simplex.Size = 2;
				Direction = XMVector3Cross(XMVector3Cross(AB, AO), AB);
				if (XMVector3LengthSq(Direction).m128_f32[0] < KINDA_SMALL)
				{
					Direction = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
				}
				return false;
			}

			// 삼각형 내부 또는 법선 방향
			if (XMVector3Dot(ABC, AO).m128_f32[0] > 0.0f)
			{
				Direction = ABC;
			}
			else
			{
				Direction = XMVectorNegate(ABC);
				// 순서 반전
				XMVECTOR Temp = Simplex.Points[1];
				Simplex.Points[1] = Simplex.Points[2];
				Simplex.Points[2] = Temp;
				Temp = Simplex.SupportPointsA[1];
				Simplex.SupportPointsA[1] = Simplex.SupportPointsA[2];
				Simplex.SupportPointsA[2] = Temp;
				Temp = Simplex.SupportPointsB[1];
				Simplex.SupportPointsB[1] = Simplex.SupportPointsB[2];
				Simplex.SupportPointsB[2] = Temp;
			}
			return false;
		}

		case 4: // 테트라헤드론
		{
			return GJKHandleTetrahedron(Simplex, Direction);
		}
	}
	return false;
}

XMVECTOR FCollisionDetector::ClosestPointOnSegmentToOrigin(FXMVECTOR P1, FXMVECTOR P2)
{
	const XMVECTOR O = XMVectorZero();
	const XMVECTOR P1P2 = XMVectorSubtract(P2, P1);
	const XMVECTOR P1O = XMVectorSubtract(O, P1); // O - P1 = -P1

	// Handle case where P1 == P2 (degenerate segment)
	float lenSq = XMVector3LengthSq(P1P2).m128_f32[0];
	if (lenSq < KINDA_SMALL)
	{
		return P1; // Return P1 if segment is a point
	}

	// Project P1O onto P1P2
	float t = XMVector3Dot(P1O, P1P2).m128_f32[0] / lenSq;

	// Clamp t to the range [0, 1] to find closest point on the segment
	t = std::clamp(t, 0.0f, 1.0f);

	// Closest point is P1 + t * (P2 - P1)
	return XMVectorAdd(P1, XMVectorScale(P1P2, t));
}

bool FCollisionDetector::GJKHandleTetrahedron(FSimplex& Simplex,XMVECTOR& Direction)
{
	// 테트라헤드론 ABCD와 원점
	XMVECTOR A = Simplex.Points[0];
	XMVECTOR B = Simplex.Points[1];
	XMVECTOR C = Simplex.Points[2];
	XMVECTOR D = Simplex.Points[3];
	XMVECTOR AO = XMVectorNegate(A);
	XMVECTOR AB = XMVectorSubtract(B, A);
	XMVECTOR AC = XMVectorSubtract(C, A);
	XMVECTOR AD = XMVectorSubtract(D, A);

	// 각 면의 법선 계산
	XMVECTOR ABC = XMVector3Cross(AB, AC);
	XMVECTOR ABD = XMVector3Cross(AB, AD);
	XMVECTOR ACD = XMVector3Cross(AD, AC);

	// 원점이 각 면의 외부인지 확인
	bool OutsideABC = XMVector3Dot(ABC, AO).m128_f32[0] > 0.0f;
	bool OutsideABD = XMVector3Dot(ABD, AO).m128_f32[0] > 0.0f;
	bool OutsideACD = XMVector3Dot(ACD, AO).m128_f32[0] > 0.0f;

	if (OutsideABC)
	{
		// ABC 면 유지
		Simplex.Points[0] = A;
		Simplex.Points[1] = B;
		Simplex.Points[2] = C;
		Simplex.SupportPointsA[0] = Simplex.SupportPointsA[0];
		Simplex.SupportPointsA[1] = Simplex.SupportPointsA[1];
		Simplex.SupportPointsA[2] = Simplex.SupportPointsA[2];
		Simplex.SupportPointsB[0] = Simplex.SupportPointsB[0];
		Simplex.SupportPointsB[1] = Simplex.SupportPointsB[1];
		Simplex.SupportPointsB[2] = Simplex.SupportPointsB[2];
		Simplex.Size = 3;
		Direction = ABC;
		return false;
	}

	if (OutsideABD)
	{
		// ABD 면 유지
		Simplex.Points[0] = A;
		Simplex.Points[1] = B;
		Simplex.Points[2] = D;
		Simplex.SupportPointsA[0] = Simplex.SupportPointsA[0];
		Simplex.SupportPointsA[1] = Simplex.SupportPointsA[1];
		Simplex.SupportPointsA[2] = Simplex.SupportPointsA[3];
		Simplex.SupportPointsB[0] = Simplex.SupportPointsB[0];
		Simplex.SupportPointsB[1] = Simplex.SupportPointsB[1];
		Simplex.SupportPointsB[2] = Simplex.SupportPointsB[3];
		Simplex.Size = 3;
		Direction = ABD;
		return false;
	}

	if (OutsideACD)
	{
		// ACD 면 유지
		Simplex.Points[0] = A;
		Simplex.Points[1] = C;
		Simplex.Points[2] = D;
		Simplex.SupportPointsA[0] = Simplex.SupportPointsA[0];
		Simplex.SupportPointsA[1] = Simplex.SupportPointsA[2];
		Simplex.SupportPointsA[2] = Simplex.SupportPointsA[3];
		Simplex.SupportPointsB[0] = Simplex.SupportPointsB[0];
		Simplex.SupportPointsB[1] = Simplex.SupportPointsB[2];
		Simplex.SupportPointsB[2] = Simplex.SupportPointsB[3];
		Simplex.Size = 3;
		Direction = ACD;
		return false;
	}

	// 원점이 테트라헤드론 내부
	return true;
}

///////////////////////////////

FCollisionDetectionResult FCollisionDetector::EPACollision(
	const ICollisionShape& ShapeA,
	const FTransform& TransformA,
	const ICollisionShape& ShapeB,
	const FTransform& TransformB,
	const FSimplex& InitialSimplex)
{
	FCollisionDetectionResult Result;
	// Start with bCollided = true, assume GJK confirmed overlap
	Result.bCollided = true;

	// EPA requires a starting simplex that encloses the origin (a tetrahedron)
	if (InitialSimplex.Size != 4) {
		LOG_FUNC_CALL("Error: Initial simplex size is not 4 (%d). EPA requires a tetrahedron.", InitialSimplex.Size);
		Result.bCollided = false; // Cannot perform EPA without a volume
		return Result;
	}

	// Initialize the polytope (convex hull) and corresponding support points
	PolytopeSOA Poly;

	// Initialize vertices from the GJK simplex
	// Reserve space to avoid reallocations during expansion
	Poly.Vertices.reserve(MaxEPAIterations + 4);
	Poly.VerticesA.reserve(MaxEPAIterations + 4);
	Poly.VerticesB.reserve(MaxEPAIterations + 4);

	for (int i = 0; i < InitialSimplex.Size; ++i) {
		Poly.Vertices.push_back(InitialSimplex.Points[i]);
		Poly.VerticesA.push_back(InitialSimplex.SupportPointsA[i]);
		Poly.VerticesB.push_back(InitialSimplex.SupportPointsB[i]);
	}

	// Define indices for the initial tetrahedron faces
	// Order is important for initial normal calculation
	static const int faceIndices[][3] = {
		{0, 1, 2}, {0, 3, 1}, {0, 2, 3}, {1, 3, 2}
	};

	// Initialize faces (indices, normals, distances) for the tetrahedron
	Poly.Indices.reserve(4 * 3); // 4 faces, 3 indices each
	Poly.Normals.reserve(4);
	Poly.Distances.reserve(4);

	for (int i = 0; i < 4; ++i) {
		// Add face indices
		Poly.Indices.push_back(faceIndices[i][0]);
		Poly.Indices.push_back(faceIndices[i][1]);
		Poly.Indices.push_back(faceIndices[i][2]);

		// Get vertex vectors using SIMD
		XMVECTOR v0 = Poly.Vertices[faceIndices[i][0]];
		XMVECTOR v1 = Poly.Vertices[faceIndices[i][1]];
		XMVECTOR v2 = Poly.Vertices[faceIndices[i][2]];

		// Calculate edge vectors using SIMD
		XMVECTOR edge1 = XMVectorSubtract(v1, v0);
		XMVECTOR edge2 = XMVectorSubtract(v2, v0);

		// Calculate potential face normal using SIMD cross product
		XMVECTOR normal = XMVector3Cross(edge1, edge2);

		// Ensure the normal points away from the origin
		// Check the dot product with a vector from a vertex to the origin (-v0).
		// If the dot product is positive, the normal points towards the origin, so flip it.
		if (XMVectorGetX(XMVector3Dot(normal, XMVectorNegate(v0))) > EPATolerance) { // Use tolerance for robustness
			normal = XMVectorNegate(normal);
			// Reverse winding order if normal is flipped to maintain consistency
			size_t lastIdx = Poly.Indices.size();
			std::swap(Poly.Indices[lastIdx - 2], Poly.Indices[lastIdx - 1]);
		}
		else if (XMVectorGetX(XMVector3Dot(normal, XMVectorNegate(v0))) < -EPATolerance) {
			// Normal is pointing away, which is good.
		}
		else {
			// Normal is close to perpendicular to the vector to origin (origin near face plane)
			// Let's assume valid initial simplex and check magnitude.
			if (XMVectorGetX(XMVector3LengthSq(normal)) < EPATolerance * EPATolerance) {
				LOG_FUNC_CALL("Error: Degenerate face found during initial polytope setup.");
				// Remove the face indices if degenerate? Or just proceed hoping EPA fixes it.
				Result.bCollided = false; // Cannot perform EPA without a volume
				return Result;
			}
		}

		// Normalize the normal using SIMD
		normal = XMVector3Normalize(normal);
		Poly.Normals.push_back(normal);

		// Calculate distance from origin to the face along the normal direction.
		// Since normal points away, this dot product should be non-negative.
		float distance = XMVectorGetX(XMVector3Dot(normal, v0));
		Poly.Distances.push_back(distance);
	}

	// EPA Iteration loop
	// We iterate to find the face closest to the origin
	int ClosestFaceIndex = -1;
	XMVECTOR ClosestNormal = XMVectorZero();
	float ClosestDistance = FLT_MAX;

	for (int Iteration = 0; Iteration < MaxEPAIterations; ++Iteration) {
		// Find the face closest to the origin in the current polytope
		ClosestFaceIndex = -1;
		ClosestDistance = FLT_MAX;

		int num_faces = Poly.Distances.size(); // Use current size as faces are added/removed
		for (int i = 0; i < num_faces; ++i) {
			// Find the minimum distance among faces whose normal points away from origin (distance >= 0)
			if (Poly.Distances[i] < ClosestDistance && Poly.Distances[i] >= -EPATolerance) { // Check >= -tolerance for robustness
				ClosestDistance = Poly.Distances[i];
				ClosestFaceIndex = i;
			}
		}

		// If no closest face found (shouldn't happen with a valid polytope)
		if (ClosestFaceIndex == -1) {
			LOG_FUNC_CALL("Error: No closest face found in EPA loop. Polytope may be invalid.");
			Result.bCollided = false; // Cannot find penetration data
			return Result;
		}

		// Get the normal of the closest face
		ClosestNormal = Poly.Normals[ClosestFaceIndex];

		// Search direction is the normal of the closest face
		XMVECTOR SearchDir = ClosestNormal;

		// Find a new support point in the search direction (farthest point on Minkowski difference)
		XMVECTOR SupportA, SupportB;
		XMVECTOR NewPoint = ComputeMinkowskiSupport(
			ShapeA, TransformA, ShapeB, TransformB, SearchDir, SupportA, SupportB);

		// Calculate the distance of the new point from the origin along the search direction
		float NewPointDistance = XMVectorGetX(XMVector3Dot(NewPoint, SearchDir));

		// Check for convergence
		// If the new point is not significantly further along the normal than the closest face's distance,
		// the polytope has expanded sufficiently. The origin is effectively on the closest face plane.
		if (std::fabs(NewPointDistance - ClosestDistance) < EPATolerance) {
			// Converged - the current closest face represents the penetration data
			XMStoreFloat3(&Result.Normal, ClosestNormal);
			Result.PenetrationDepth = ClosestDistance; // Use the distance of the closest face

			// Calculate contact point
			// A common approximation is the midpoint between the average of support points on A
			// and the average of support points on B for the vertices of the closest face.
			int faceStartIdx = ClosestFaceIndex * 3;
			XMVECTOR ContactPointA_Sum = XMVectorZero();
			XMVECTOR ContactPointB_Sum = XMVectorZero();

			for (int i = 0; i < 3; ++i) {
				int vertexIndex = Poly.Indices[faceStartIdx + i];
				// Accumulate corresponding support points from Poly
				ContactPointA_Sum = XMVectorAdd(ContactPointA_Sum, Poly.VerticesA[vertexIndex]);
				ContactPointB_Sum = XMVectorAdd(ContactPointB_Sum, Poly.VerticesB[vertexIndex]);
			}

			// Average the sums
			XMVECTOR ContactPointA_Avg = XMVectorScale(ContactPointA_Sum, 1.0f / 3.0f);
			XMVECTOR ContactPointB_Avg = XMVectorScale(ContactPointB_Sum, 1.0f / 3.0f);

			// Approximate the contact point as the midpoint between the averaged support points
			XMVECTOR ResultContactPoint = XMVectorScale(XMVectorAdd(ContactPointA_Avg, ContactPointB_Avg), 0.5f);

			XMStoreFloat3(&Result.Point, ResultContactPoint);

			// Successfully found penetration data
			return Result; // Exit EPA function early
		}

		// If not converged, add the new point to the polytope vertices
		int NewPointIndex = static_cast<int>(Poly.Vertices.size());
		Poly.Vertices.push_back(NewPoint);
		Poly.VerticesA.push_back(SupportA); // Store corresponding support points
		Poly.VerticesB.push_back(SupportB);

		// Expand the polytope by removing faces visible from the new point
		// and creating new faces connecting the new point to the horizon edges.
		UpdatePolytope(Poly, NewPointIndex);
	}

	// If the loop finishes without converging (reached MaxEPAIterations)
	// Use the data from the closest face found in the *last* iteration.
	// ClosestFaceIndex, ClosestNormal, and ClosestDistance still hold this data.
	// If ClosestFaceIndex is -1, it means an error occurred inside the loop (already handled).
	// If we reach here, ClosestFaceIndex should be valid from the last iteration.
	LOG_FUNC_CALL("Warning: EPA reached max iterations without full convergence. Using last closest face data.");

	// Populate result with data from the last closest face
	XMStoreFloat3(&Result.Normal, ClosestNormal);
	Result.PenetrationDepth = ClosestDistance;

	// Recalculate contact point using the last closest face's data (indices might change if UpdatePolytope reordered, but face indices are stable for a given face)
	// Re-finding the indices here ensures they match the final Poly.Indices
	ClosestFaceIndex = -1;
	float FinalClosestDistance = FLT_MAX;
	int num_faces = Poly.Distances.size();
	for (int i = 0; i < num_faces; ++i) {
		if (Poly.Distances[i] < FinalClosestDistance && Poly.Distances[i] >= -EPATolerance) {
			FinalClosestDistance = Poly.Distances[i];
			ClosestFaceIndex = i;
		}
	}

	if (ClosestFaceIndex != -1) { // Should be found if loop didn't error
		XMStoreFloat3(&Result.Normal, Poly.Normals[ClosestFaceIndex]); // Use the final normal
		Result.PenetrationDepth = Poly.Distances[ClosestFaceIndex]; // Use the final distance

		int faceStartIdx = ClosestFaceIndex * 3;
		XMVECTOR ContactPointA_Sum = XMVectorZero();
		XMVECTOR ContactPointB_Sum = XMVectorZero();

		for (int i = 0; i < 3; ++i) {
			int vertexIndex = Poly.Indices[faceStartIdx + i];
			ContactPointA_Sum = XMVectorAdd(ContactPointA_Sum, Poly.VerticesA[vertexIndex]);
			ContactPointB_Sum = XMVectorAdd(ContactPointB_Sum, Poly.VerticesB[vertexIndex]);
		}

		XMVECTOR ContactPointA_Avg = XMVectorScale(ContactPointA_Sum, 1.0f / 3.0f);
		XMVECTOR ContactPointB_Avg = XMVectorScale(ContactPointB_Sum, 1.0f / 3.0f);

		XMVECTOR ResultContactPoint = XMVectorScale(XMVectorAdd(ContactPointA_Avg, ContactPointB_Avg), 0.5f);
		XMStoreFloat3(&Result.Point, ResultContactPoint);

	}
	else {
		LOG_FUNC_CALL("Error: Could not find closest face after max iterations.");
		Result.bCollided = false; // Indicate failure if final face lookup fails
	}

	return Result;
}

void FCollisionDetector::CalculateFaceNormalAndDistance(const PolytopeSOA& Poly, int face_index, DirectX::XMVECTOR& out_normal, float& out_distance) const
{
	// Calculate the normal and distance for a given face
	// The normal is oriented to point away from the origin

	// Get vertex indices for the triangle
	int i0 = Poly.Indices[face_index * 3];
	int i1 = Poly.Indices[face_index * 3 + 1];
	int i2 = Poly.Indices[face_index * 3 + 2];

	// Get vertex coordinates using XMVectorLoadFloat3 (or direct use if already XMVECTOR)
	DirectX::XMVECTOR v0 = Poly.Vertices[i0];
	DirectX::XMVECTOR v1 = Poly.Vertices[i1];
	DirectX::XMVECTOR v2 = Poly.Vertices[i2];

	// Calculate edge vectors using SIMD
	DirectX::XMVECTOR edge1 = DirectX::XMVectorSubtract(v1, v0);
	DirectX::XMVECTOR edge2 = DirectX::XMVectorSubtract(v2, v0);

	// Calculate potential normal using SIMD cross product
	DirectX::XMVECTOR n_candidate = DirectX::XMVector3Cross(edge1, edge2);

	// Normalize the normal using SIMD
	n_candidate = DirectX::XMVector3Normalize(n_candidate);

	// Check orientation relative to the origin using SIMD dot product
	DirectX::XMVECTOR dot_origin_v = DirectX::XMVector3Dot(n_candidate, v0);
	float dot_origin = DirectX::XMVectorGetX(dot_origin_v);

	// Ensure the normal points away from the origin
	if (dot_origin < 0)
	{
		out_normal = DirectX::XMVectorNegate(n_candidate);
	}
	else
	{
		out_normal = n_candidate;
	}

	// Calculate the distance from the origin
	DirectX::XMVECTOR distance_v = DirectX::XMVector3Dot(out_normal, v0);
	out_distance = DirectX::XMVectorGetX(distance_v);
}

bool FCollisionDetector::IsFaceVisible(const PolytopeSOA& Poly, int face_index, const DirectX::XMVECTOR& new_point_vector, const DirectX::XMVECTOR& face_normal, float face_distance) const
{
	// Check if a face is visible from a new point
	// Visibility is determined by the sign of the dot product between the face normal
	// and the vector from a point on the face to the new point.

	// Get a vertex on the face (any vertex will do, using the first one)
	int v_idx = Poly.Indices[face_index * 3];
	DirectX::XMVECTOR v_on_face = Poly.Vertices[v_idx];

	// Calculate the vector from the face vertex to the new point using SIMD
	DirectX::XMVECTOR vec_to_new_point = DirectX::XMVectorSubtract(new_point_vector, v_on_face);

	// Calculate the dot product using SIMD
	DirectX::XMVECTOR dot_product_v = DirectX::XMVector3Dot(vec_to_new_point, face_normal);
	float dot_product = DirectX::XMVectorGetX(dot_product_v);

	// The face is visible if the dot product is positive within tolerance
	return dot_product > EPATolerance;
}

std::vector<std::pair<int, int>> FCollisionDetector::BuildHorizonEdges(const PolytopeSOA& Poly, const std::vector<int>& visible_face_indices) const
{
	// Find horizon edges separating visible faces from non-visible faces
	// Horizon edges are those shared by exactly one visible face.

	std::unordered_map<Edge, int, EdgeHash> edge_counts;
	std::vector<std::pair<int, int>> horizon_edges;

	// Iterate through all visible faces
	for (int face_index : visible_face_indices)
	{
		// Get vertex indices for the triangle
		int i0 = Poly.Indices[face_index * 3];
		int i1 = Poly.Indices[face_index * 3 + 1];
		int i2 = Poly.Indices[face_index * 3 + 2];

		// Create Edge objects (constructor handles sorting indices)
		Edge e0(i0, i1);
		Edge e1(i1, i2);
		Edge e2(i2, i0);

		// Increment count for each edge
		edge_counts[e0]++;
		edge_counts[e1]++;
		edge_counts[e2]++;
	}

	// Iterate through edge counts to find edges that appeared only once
	for (const auto& pair : edge_counts)
	{
		if (pair.second == 1)
		{
			// This edge is on the boundary (horizon)
			horizon_edges.push_back({ pair.first.Start, pair.first.End });
		}
	}

	return horizon_edges;
}

void FCollisionDetector::CreateNewFaces(const std::vector<std::pair<int, int>>& horizon_edges, int new_point_index, const PolytopeSOA& original_poly, std::vector<int>& new_indices, std::vector<DirectX::XMVECTOR>& new_normals, std::vector<float>& new_distances) const
{
	// Create new triangular faces by connecting the new point to each horizon edge

	// Get the vector for the new point
	DirectX::XMVECTOR new_point_v = original_poly.Vertices[new_point_index];

	for (const auto& edge : horizon_edges)
	{
		int v0_idx = edge.first;
		int v1_idx = edge.second;

		// Add indices for the new triangle (v0, v1, new_point)
		new_indices.push_back(v0_idx);
		new_indices.push_back(v1_idx);
		new_indices.push_back(new_point_index);

		// Get vertices using XMVectorLoadFloat3 (or direct use if already XMVECTOR)
		DirectX::XMVECTOR v0 = original_poly.Vertices[v0_idx];
		DirectX::XMVECTOR v1 = original_poly.Vertices[v1_idx];

		// Calculate edge vectors for the new face using SIMD
		DirectX::XMVECTOR edge1 = DirectX::XMVectorSubtract(v1, v0);
		DirectX::XMVECTOR edge2 = DirectX::XMVectorSubtract(new_point_v, v0);

		// Calculate potential normal using SIMD cross product
		DirectX::XMVECTOR n_candidate = DirectX::XMVector3Cross(edge1, edge2);

		// Normalize the normal using SIMD
		n_candidate = DirectX::XMVector3Normalize(n_candidate);

		// Check orientation relative to the origin using SIMD dot product
		DirectX::XMVECTOR dot_origin_v = DirectX::XMVector3Dot(n_candidate, v0);
		float dot_origin = DirectX::XMVectorGetX(dot_origin_v);

		// Determine final normal: Must point away from the origin
		DirectX::XMVECTOR new_normal;
		if (dot_origin < 0)
		{
			new_normal = DirectX::XMVectorNegate(n_candidate);
		}
		else
		{
			new_normal = n_candidate;
		}


		// Calculate the distance from the origin for the new face
		DirectX::XMVECTOR distance_v = DirectX::XMVector3Dot(new_normal, v0);
		float new_distance = DirectX::XMVectorGetX(distance_v);

		// Add the calculated normal and distance
		new_normals.push_back(new_normal);
		new_distances.push_back(new_distance);
	}
}

void FCollisionDetector::UpdatePolytope(PolytopeSOA& Poly, int NewPointIndex)
{
	// Temporary storage for the indices of faces visible from the new point
	std::vector<int> visible_face_indices;

	// Get the vector for the new point (already exists in Poly.Vertices)
	DirectX::XMVECTOR new_point_vector = Poly.Vertices[NewPointIndex];

	// Iterate through current faces to identify visible ones
	int num_faces = Poly.Indices.size() / 3;

	for (int i = 0; i < num_faces; ++i)
	{
		//가시성 검사
		DirectX::XMVECTOR face_normal = Poly.Normals[i];
		float face_distance = Poly.Distances[i];

		//// Recalculate normal/distance *only* for the visibility check
		//CalculateFaceNormalAndDistance(Poly, i, face_normal, face_distance); // Recalculate based on current vertex positions for check

		if (IsFaceVisible(Poly, i, new_point_vector, face_normal, face_distance)) // Use recalculated data for check
		{
			// This face is visible and will be removed
			visible_face_indices.push_back(i);
		}

	}

	// Temporary storage for the data of the next polytope (non-visible + new faces)
	std::vector<int> next_indices;
	std::vector<DirectX::XMVECTOR> next_normals;
	std::vector<float> next_distances;

	// Reserve approximate space: Number of non-visible faces + number of new faces (horizon edges count)
	// Max horizon edges is roughly (visible_face_count * 3) / 2
	int num_visible = visible_face_indices.size();
	int num_non_visible = num_faces - num_visible;
	// Estimating new faces is hard, let's just reserve some space
	next_indices.reserve(num_non_visible * 3 + num_visible * 3); // Pessimistic
	next_normals.reserve(num_non_visible + num_visible * 2); // Pessimistic
	next_distances.reserve(num_non_visible + num_visible * 2); // Pessimistic


	// Add the non-visible faces from the *original* polytope data
	std::sort(visible_face_indices.begin(), visible_face_indices.end()); // Sort to use binary search

	for (int i = 0; i < num_faces; ++i)
	{
		// If this face index is NOT in the visible_face_indices list
		if (!std::binary_search(visible_face_indices.begin(), visible_face_indices.end(), i))
		{
			// This face is not visible and will be part of the new polytope.
			// Use the NORMAL and DISTANCE ALREADY STORED in Poly.
			next_indices.push_back(Poly.Indices[i * 3]);
			next_indices.push_back(Poly.Indices[i * 3 + 1]);
			next_indices.push_back(Poly.Indices[i * 3 + 2]);
			next_normals.push_back(Poly.Normals[i]);   // <-- Use stored normal
			next_distances.push_back(Poly.Distances[i]); // <-- Use stored distance
		}
	}


	// Find the horizon edges from the visible faces
	std::vector<std::pair<int, int>> horizon_edges = BuildHorizonEdges(Poly, visible_face_indices);

	// Create new faces connecting the new point to the horizon edges
	// CreateNewFaces calculates normals and distances for these new faces.
	CreateNewFaces(horizon_edges, NewPointIndex, Poly, next_indices, next_normals, next_distances);

	// Replace the old polytope data with the new data
	Poly.Indices = std::move(next_indices);
	Poly.Normals = std::move(next_normals);
	Poly.Distances = std::move(next_distances);
	// Poly.Vertices and ContactData.VerticesA/B are not modified here,
	// they only grow when a NewPoint is added in EPACollision.
}
#pragma endregion

#pragma region DEBUG
//debug
void FCollisionDetector::DrawPolytope(const FCollisionDetector::PolytopeSOA& Polytope,
									  float LifeTime = 0.1f, bool bDrawNormals = false, 
									  const Vector4& Color = Vector4(1, 1, 1, 1))
{
	using PolytopeSOA = FCollisionDetector::PolytopeSOA;

	if (Polytope.Indices.empty() || Polytope.Vertices.empty())
		return;

	auto* DebugDrawer = UDebugDrawManager::Get();
	if (!DebugDrawer)
		return;

	// 각 면(triangle)마다 처리
	for (size_t i = 0; i < Polytope.Indices.size(); i += 3)
	{
		if (i + 2 >= Polytope.Indices.size())
			break; // 안전 검사

		// 삼각형의 세 꼭지점 인덱스
		int IdxA = Polytope.Indices[i];
		int IdxB = Polytope.Indices[i + 1];
		int IdxC = Polytope.Indices[i + 2];

		// 인덱스 유효성 검사
		if (IdxA >= Polytope.Vertices.size() || IdxB >= Polytope.Vertices.size() || IdxC >= Polytope.Vertices.size() ||
			IdxA < 0 || IdxB < 0 || IdxC < 0)
			continue;

		// 삼각형 꼭지점 좌표
		Vector3 VertA, VertB, VertC;
		XMStoreFloat3(&VertA, Polytope.Vertices[IdxA]);
		XMStoreFloat3(&VertB, Polytope.Vertices[IdxB]);
		XMStoreFloat3(&VertC, Polytope.Vertices[IdxC]);

		// 삼각형 외곽선 그리기
		DebugDrawer->DrawLine(VertA, VertB, Color, 0.001f, LifeTime);
		DebugDrawer->DrawLine(VertB, VertC, Color, 0.001f, LifeTime);
		DebugDrawer->DrawLine(VertC, VertA, Color, 0.001f, LifeTime);
	}

	// 추가적으로 면의 법선 시각화 (선택적)
	if (bDrawNormals && Polytope.Normals.size() * 3 >= Polytope.Indices.size())
	{
		Vector4 InvalidNormalColor = Vector4(1.0f, 0.0f, 0.0f, 1.0f); // Red
		Vector4 ValidNormalColor = Vector4(0.0f, 0.0f, 1.0f, 1.0f); // Blue
		float NormalLength = 0.2f; // 법선 길이

		for (size_t i = 0; i < Polytope.Normals.size(); i++)
		{
			// 삼각형의 중심점 계산
			size_t TriIdx = i * 3;
			if (TriIdx + 2 >= Polytope.Indices.size())
				break;

			int IdxA = Polytope.Indices[TriIdx];
			int IdxB = Polytope.Indices[TriIdx + 1];
			int IdxC = Polytope.Indices[TriIdx + 2];

			if (IdxA >= Polytope.Vertices.size() || IdxB >= Polytope.Vertices.size() || IdxC >= Polytope.Vertices.size() ||
				IdxA < 0 || IdxB < 0 || IdxC < 0)
				continue;

			// 삼각형 중심 계산
			XMVECTOR TriCenter = XMVectorScale(
				XMVectorAdd(XMVectorAdd(Polytope.Vertices[IdxA], Polytope.Vertices[IdxB]), Polytope.Vertices[IdxC]),
				1.0f / 3.0f);

			//법선 원점 방향 검사
			Vector4 NormalColor = ValidNormalColor;
			// 법선이 원점을 향하면 INvlaid
			XMVECTOR toOrigin = XMVectorNegate(Polytope.Vertices[IdxA]);
			if (XMVectorGetX(XMVector3Dot(Polytope.Normals[i], toOrigin)) > KINDA_SMALL) {
				NormalColor = InvalidNormalColor;
			}

			// 법선 벡터 계산
			XMVECTOR NormalEnd = XMVectorAdd(TriCenter, XMVectorScale(Polytope.Normals[i], NormalLength));

			// 법선 그리기
			Vector3 Start, End;
			XMStoreFloat3(&Start, TriCenter);
			XMStoreFloat3(&End, NormalEnd);
			DebugDrawer->DrawLine(Start, End, NormalColor, 0.001f, LifeTime);
		}
	}
}
#pragma endregion