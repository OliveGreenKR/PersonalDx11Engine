#include "CollisionDetector.h"
#include <algorithm>
#include "ConfigReadManager.h"
#include "DebugDrawerManager.h"
#include <map>/
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
	Direction = XMVectorNegate(Point);

	// GJK 반복
	for (int Iteration = 0; Iteration < MaxGJKIterations; ++Iteration)
	{
		// 새로운 지원점 계산
		Point = ComputeMinkowskiSupport(
			ShapeA, TransformA, ShapeB, TransformB, Direction, SupportA, SupportB);

		// 원점을 지나지 못하면 충돌 없음
		if (XMVector3Dot(Point, Direction).m128_f32[0] <= 0.0f)
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
	Vector3 SupportA = ShapeA.GetLocalSupportPoint(NegDir);
	OutSupportA = XMLoadFloat3(&SupportA);

	// ShapeB의 지원점 계산
	Vector3 SupportB = ShapeB.GetLocalSupportPoint(Dir);
	OutSupportB = XMLoadFloat3(&SupportB);

	// Minkowski 차분 계산
	return XMVectorSubtract(OutSupportB, OutSupportA);
}

bool FCollisionDetector::UpdateSimplex(FSimplex& Simplex, XMVECTOR& Direction)
{
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
	}

	return false;
}

///////////////////////////////

FCollisionDetectionResult FCollisionDetector::EPACollision(
	const ICollisionShape& ShapeA,
	const FTransform& TransformA,
	const ICollisionShape& ShapeB,
	const FTransform& TransformB,
	const FSimplex& Simplex)
{
	FCollisionDetectionResult Result;

	
	return Result;
}

#pragma endregion