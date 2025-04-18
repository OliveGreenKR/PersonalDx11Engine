#include "CollisionDetector.h"
#include <algorithm>
#include "ConfigReadManager.h"

FCollisionDetector::FCollisionDetector()
{
	UConfigReadManager::Get()->GetValue("MaxGJKIteration", GJK_MAX_ITERATION);
	UConfigReadManager::Get()->GetValue("MaxEpaIteration", EPA_MAX_ITERATIONS);
	UConfigReadManager::Get()->GetValue("CCDTimeStep", CCDTimeStep);
	UConfigReadManager::Get()->GetValue("GJKEpsilon", GJK_EPSILON);
	UConfigReadManager::Get()->GetValue("EPAEpsilon", EPA_EPSILON);
}

FCollisionDetectionResult FCollisionDetector::DetectCollisionDiscrete(
	const ICollisionShape& ShapeA,
	const FTransform& TransformA,
	const ICollisionShape& ShapeB,
	const FTransform& TransformB)
{
	//return DetectCollisionShapeBasedDiscrete(ShapeA, TransformA,
	//								  ShapeB, TransformB);
	return DetectCollisionGJKEPADiscrete(ShapeA, TransformA,
											 ShapeB, TransformB);

	return FCollisionDetectionResult();  // 지원하지 않는 조합
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
	FCollisionDetectionResult StartResult = DetectCollisionDiscrete(ShapeA, PrevTransformA, ShapeB, PrevTransformB);
	if (StartResult.bCollided)
	{
		StartResult.TimeOfImpact = 0.0f;  // 시작부터 충돌 상태
		return StartResult;
	}

	FCollisionDetectionResult EndResult = DetectCollisionDiscrete(ShapeA, CurrentTransformA, ShapeB, CurrentTransformB);
	if (!EndResult.bCollided)
	{
		// 시작과 끝 모두 충돌하지 않으며, 경로가 매우 짧은 경우 -> 충돌 검사 건너띄기
		Vector3 RelativeMotion = (CurrentTransformA.Position - PrevTransformA.Position) -
			(CurrentTransformB.Position - PrevTransformB.Position);
		if (RelativeMotion.LengthSquared() < KINDA_SMALL)
		{
			return EndResult;
		}
	}

	float currentTime = 0.0f;

	FCollisionDetectionResult Result;
	Result.bCollided = false;

	while (currentTime < DeltaTime)
	{
		float alpha = currentTime / DeltaTime;

		// 중간 시점의 Transform 계산
		FTransform InterpolatedTransformA = FTransform::InterpolateTransform(PrevTransformA, CurrentTransformA, alpha);
		FTransform InterpolatedTransformB = FTransform::InterpolateTransform(PrevTransformB, CurrentTransformB, alpha);

		//최초 충돌 시점 찾기
		Result = DetectCollisionDiscrete(ShapeA, InterpolatedTransformA, ShapeB, InterpolatedTransformB);

		if (Result.bCollided)
		{
			Result.TimeOfImpact = currentTime;
			break;
		}

		currentTime += CCDTimeStep;
	}

	return EndResult;  // 최종 상태의 결과 반환
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

FCollisionDetectionResult FCollisionDetector::DetectCollisionGJKEPADiscrete(const ICollisionShape& ShapeA, const FTransform& TransformA,
																			const ICollisionShape& ShapeB, const FTransform& TransformB)
{
	FCollisionDetectionResult Result;

	// 단체 초기화
	FSimplex Simplex;
	Simplex.Size = 0;

	// GJK로 충돌 여부 판단
	if (GJK(ShapeA, TransformA, ShapeB, TransformB, Simplex)) {
		// 충돌 발생 - EPA로 침투 깊이와 법선 계산
		Result = EPA(ShapeA, TransformA, ShapeB, TransformB, Simplex);
	}
	else {
		// 충돌 없음
		Result.bCollided = false;
	}

	return Result;
}

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
	if (penetrationDepth <= 0.0f)
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

//////////////////////////////////////

Vector3 FCollisionDetector::Support(
	const ICollisionShape& ShapeA, const FTransform& TransformA,
	const ICollisionShape& ShapeB, const FTransform& TransformB,
	const Vector3& Direction)
{
	// A에서 방향으로 가장 멀리 있는 점
	Vector3 SupportA = ShapeA.GetSupportPoint(Direction,TransformA);

	// B에서 반대 방향으로 가장 멀리 있는 점
	Vector3 SupportB = ShapeB.GetSupportPoint(-Direction, TransformB);

	// 민코프스키 차에서의 점 (A - B)
	return SupportA - SupportB;
}

bool FCollisionDetector::GJK(
	const ICollisionShape& ShapeA, const FTransform& TransformA,
	const ICollisionShape& ShapeB, const FTransform& TransformB,
	FSimplex& OutSimplex)
{
	// 초기 방향 (임의 설정)
	Vector3 Direction = TransformB.Position - TransformA.Position;
	if (Direction.LengthSquared() < KINDA_SMALL)
		Direction = Vector3(1.0f, 0.0f, 0.0f); // fallback

	// 첫 번째 점 얻기
	Vector3 Support = FCollisionDetector::Support(ShapeA, TransformA, ShapeB, TransformB, Direction);

	// 심플렉스 초기화
	OutSimplex.Points[0] = Support;
	OutSimplex.Size = 1;

	// 다음 방향은 원점 방향
	Direction = -Support;

	for (int i = 0; i < GJK_MAX_ITERATION; i++)
	{
		// 새로운 지원점 얻기
		Support = FCollisionDetector::Support(ShapeA, TransformA, ShapeB, TransformB, Direction);

		// 새 지원점이 방향에서 원점을 넘어가지 않으면 충돌 없음
		if (Vector3::Dot(Support, Direction) < 0)
		{
			return false;
		}

		// 심플렉스에 점 추가
		OutSimplex.Points[OutSimplex.Size++] = Support;

		// 심플렉스 처리 및 새 방향 계산
		if (ProcessSimplex(OutSimplex, Direction))
		{
			return true; // 원점 포함, 충돌 감지
		}
	}

	// 최대 반복 횟수 초과
	return false;
}

bool FCollisionDetector::ProcessLine(XMVECTOR* Points, FSimplex& Simplex, Vector3& Direction)
{
	// A = 가장 최근 추가된 점, B = 이전 점
	XMVECTOR A = Points[0]; // 최근 점
	XMVECTOR B = Points[1]; // 이전 점

	// AB 벡터와 AO 벡터 계산
	XMVECTOR AB = XMVectorSubtract(B, A);
	XMVECTOR AO = XMVectorNegate(A); // A에서 원점(0)으로의 벡터

	// AB 방향으로의 AO 투영 검사
	if (XMVectorGetX(XMVector3Dot(AB, AO)) > 0.0f) {
		// 원점이 AB 방향에 있음
		// AB × AO × AB 방향으로 새 방향 설정
		XMVECTOR Cross1 = XMVector3Cross(AB, AO);
		XMVECTOR NewDir = XMVector3Cross(Cross1, AB);

		// Normalize only if non-zero
		XMVECTOR LengthSq = XMVector3LengthSq(NewDir);
		if (XMVectorGetX(LengthSq) > KINDA_SMALL) {
			XMStoreFloat3(&Direction, XMVector3Normalize(NewDir));
		}
		else {
			XMStoreFloat3(&Direction, AO); // Fallback to AO
		}
	}
	else {
		// 원점이 A 방향에 있음
		XMStoreFloat3(&Direction, AO);
	}
	return false;
}

bool FCollisionDetector::ProcessTriangle(XMVECTOR* Points, FSimplex& Simplex, Vector3& Direction)
{
	// A = 가장 최근 추가된 점, B, C = 이전 점들
	XMVECTOR A = Points[0]; // 최근 점
	XMVECTOR B = Points[1];
	XMVECTOR C = Points[2];

	// 각 변의 벡터 계산
	XMVECTOR AB = XMVectorSubtract(B, A);
	XMVECTOR AC = XMVectorSubtract(C, A);
	XMVECTOR AO = XMVectorNegate(A); // A에서 원점으로

	// 삼각형의 법선 계산
	XMVECTOR Normal = XMVector3Cross(AB, AC);
	XMVECTOR NormalLengthSq = XMVector3LengthSq(Normal);
	if (XMVectorGetX(NormalLengthSq) < KINDA_SMALL) {
		// Degenerate triangle, fallback to point
		Simplex.Size = 1;
		Simplex.Points[1] = Simplex.Points[0];
		XMStoreFloat3(&Direction, AO);
		return false;
	}
	Normal = XMVector3Normalize(Normal);

	// 영역 테스트
	XMVECTOR ABC_Normal = XMVector3Cross(AB, AC);
	XMVECTOR ABPerp = XMVector3Cross(AB, ABC_Normal);
	XMVECTOR ACPerp = XMVector3Cross(ABC_Normal, AC);

	float ABDot = XMVectorGetX(XMVector3Dot(ABPerp, AO));
	float ACDot = XMVectorGetX(XMVector3Dot(ACPerp, AO));

	if (ABDot > 0.0f && ACDot > 0.0f) {
		// 원점이 삼각형 위에 있음 - 평면 상의 테스트 필요
		// ABC 평면의 법선 방향과 원점 방향 비교
		if (XMVectorGetX(XMVector3Dot(Normal, AO)) > 0) {
			// 원점이 법선 방향에 있음 (삼각형 앞)
			XMStoreFloat3(&Direction, Normal);
		}
		else {
			// 원점이 법선 반대 방향에 있음 (삼각형 뒤)
			XMStoreFloat3(&Direction, XMVectorNegate(Normal));
			// 점 순서 조정 (볼록 다면체 방향 일관성 유지)
			std::swap(Simplex.Points[1], Simplex.Points[2]);
		}
		return false;
	}

	// 변 AB 테스트
	if (ABDot > 0.0f) {
		// 원점이 AB 근처
		Simplex.Points[2] = Simplex.Points[1]; // C = B
		Simplex.Points[1] = Simplex.Points[0]; // B = A
		Simplex.Size = 2;

		// AB 수직 방향으로 새 방향 설정
		XMStoreFloat3(&Direction, XMVector3Normalize(ABPerp));
		return false;
	}

	// 변 AC 테스트
	if (ACDot > 0.0f) {
		// 원점이 AC 근처
		Simplex.Points[1] = Simplex.Points[0]; // B = A
		Simplex.Size = 2;

		// AC 수직 방향으로 새 방향 설정
		XMStoreFloat3(&Direction, XMVector3Normalize(ACPerp));
		return false;
	}

	// 원점은 A 근처에 있음
	Simplex.Points[1] = Simplex.Points[0]; // B = A
	Simplex.Size = 1;
	XMStoreFloat3(&Direction, AO);
	return false;
}

bool FCollisionDetector::ProcessTetrahedron(XMVECTOR* Points, FSimplex& Simplex, Vector3& Direction)
{
	// A = 가장 최근 추가된 점, B, C, D = 이전 점들
	XMVECTOR A = Points[0]; // 최근 점
	XMVECTOR B = Points[1];
	XMVECTOR C = Points[2];
	XMVECTOR D = Points[3];

	XMVECTOR AO = XMVectorNegate(A); // A에서 원점으로

	// 사면체의 각 삼각형 평면 계산
	// 각 삼각형의 법선은 사면체 외부를 향해야 함
	XMVECTOR ABC = XMVector3Cross(XMVectorSubtract(B, A), XMVectorSubtract(C, A));
	XMVECTOR ACD = XMVector3Cross(XMVectorSubtract(C, A), XMVectorSubtract(D, A));
	XMVECTOR ADB = XMVector3Cross(XMVectorSubtract(D, A), XMVectorSubtract(B, A));

	// 원점의 각 평면에 대한 위치 테스트
	bool InFrontOfABC = XMVectorGetX(XMVector3Dot(ABC, AO)) > 0;
	bool InFrontOfACD = XMVectorGetX(XMVector3Dot(ACD, AO)) > 0;
	bool InFrontOfADB = XMVectorGetX(XMVector3Dot(ADB, AO)) > 0;

	// 모든 평면 뒤에 있으면, 원점은 사면체 내부에 있음
	if (!InFrontOfABC && !InFrontOfACD && !InFrontOfADB) {
		// 마지막 평면 (BDC) 테스트
		XMVECTOR BDC = XMVector3Cross(XMVectorSubtract(D, B), XMVectorSubtract(C, B));
		XMVECTOR BO = XMVectorNegate(B); // B에서 원점으로

		if (XMVectorGetX(XMVector3Dot(BDC, BO)) <= 0) {
			// 원점이 사면체 내부에 있음
			return true;
		}
	}

	// 원점이 사면체 외부면, 가장 가까운 특성을 찾아 단체 크기를 줄임
	// ABC 평면 테스트
	if (InFrontOfABC) {
		// D 제거
		Simplex.Points[3] = Simplex.Points[2]; // D = C
		Simplex.Points[2] = Simplex.Points[1]; // C = B
		Simplex.Points[1] = Simplex.Points[0]; // B = A
		Simplex.Size = 3;
		XMStoreFloat3(&Direction, ABC);
		return false;
	}

	// ACD 평면 테스트
	if (InFrontOfACD) {
		// B 제거
		Simplex.Points[1] = Simplex.Points[0]; // B = A
		Simplex.Points[2] = Simplex.Points[2]; // C 유지
		Simplex.Points[3] = Simplex.Points[3]; // D 유지
		Simplex.Size = 3;
		XMStoreFloat3(&Direction, ACD);
		return false;
	}

	// ADB 평면 테스트
	if (InFrontOfADB) {
		// C 제거
		Simplex.Points[1] = Simplex.Points[0]; // B = A
		Simplex.Points[2] = Simplex.Points[3]; // C = D
		Simplex.Size = 3;
		XMStoreFloat3(&Direction, ADB);
		return false;
	}

	// 코드가 여기까지 오면 뭔가 잘못됨 - 모든 평면 뒤에 있는 경우 이미 리턴했어야 함
	LOG_FUNC_CALL("[WARNING] Processing Simplex is considered incorrect");
	return false;
}

bool FCollisionDetector::ProcessPoint(XMVECTOR* Points, FSimplex& Simplex, Vector3& Direction)
{
	// 단일 점의 경우 원점 방향으로 설정
	XMStoreFloat3(&Direction, XMVectorNegate(Points[0]));
	return false;
}

bool FCollisionDetector::ProcessSimplex(FSimplex& Simplex, Vector3& Direction)
{
	// Ensure valid simplex size
	if (Simplex.Size < 1 || Simplex.Size > 4) {
		LOG_FUNC_CALL("[ERROR] Invalid Simplex size");
		XMStoreFloat3(&Direction, XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f));
		return false;
	}

	// SIMD 벡터 배열 생성
	XMVECTOR Points[4] = {};
	for (int i = 0; i < Simplex.Size; i++) {
		Points[i] = XMLoadFloat3(&Simplex.Points[i]);
	}

	switch (Simplex.Size)
	{
		case 1: // 점(Point)
			return ProcessPoint(Points, Simplex, Direction);
		case 2: // 선(Line)
			return ProcessLine(Points, Simplex, Direction);
		case 3: // 삼각형(Triangle)
			return ProcessTriangle(Points, Simplex, Direction);
		case 4: // 사면체(Tetrahedron)
			return ProcessTetrahedron(Points, Simplex, Direction);
		default: // 예상치 못한 케이스
			LOG_FUNC_CALL("[ERROR] Unreachable Simplex size");
			XMStoreFloat3(&Direction, XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f)); // 임의 방향
			return false;
	}
}

/////////////////////////////
FCollisionDetectionResult FCollisionDetector::EPA(
	const ICollisionShape& ShapeA, const FTransform& TransformA,
	const ICollisionShape& ShapeB, const FTransform& TransformB,
	FSimplex& Simplex)
{
	FCollisionDetectionResult Result;
	Result.bCollided = true;

	// 내부 함수로 에지 인덱스 조회
	auto GetEdgeIndex = [](int a, int b) -> int {
		return (a << 16) | b;
		};

	// 초기 다면체 구축
	std::vector<FFace> Faces = BuildInitialPolyhedron(Simplex);
	if (Faces.empty()) {
		LOG_FUNC_CALL("[ERROR] Failed to build initial polyhedron");
		Result.bCollided = false;
		return Result;
	}

	// 가장 가까운 면 찾기 (원점에 가장 가까운 면)
	int ClosestFaceIndex = FindClosestFace(Faces);
	if (ClosestFaceIndex < 0) {
		LOG_FUNC_CALL("[ERROR] No valid faces found");
		Result.bCollided = false;
		return Result;
	}

	// EPA 반복 수행
	for (int Iteration = 0; Iteration < EPA_MAX_ITERATIONS; ++Iteration)
	{
		FFace& ClosestFace = Faces[ClosestFaceIndex];

		// 침투 깊이가 충분히 정확하면 종료
		if (ClosestFace.Distance < EPA_EPSILON) {
			break;
		}

		// 가장 가까운 면의 법선 방향으로 새 지원점 찾기
		Vector3 SearchDir;
		XMStoreFloat3(&SearchDir, XMLoadFloat3(&ClosestFace.Normal));
		Vector3 NewPoint = Support(ShapeA, TransformA, ShapeB, TransformB, SearchDir);

		// 새 점과 원점 사이의 거리 계산
		XMVECTOR vNewPoint = XMLoadFloat3(&NewPoint);
		XMVECTOR vNormal = XMLoadFloat3(&ClosestFace.Normal);
		float Distance = XMVectorGetX(XMVector3Dot(vNewPoint, vNormal));

		// 거리 차이가 충분히 작으면 더 이상 확장할 필요 없음
		if (std::abs(Distance - ClosestFace.Distance) < EPA_EPSILON) {
			// 수렴 - 결과 설정 후 종료
			Result.Normal = ClosestFace.Normal;
			Result.PenetrationDepth = ClosestFace.Distance;

			// 충돌 지점은 보통 접촉 다면체의 무게중심으로 설정
			Vector3 ContactPoint = Vector3::Zero;
			for (int i = 0; i < 3; ++i) {
				int Idx = ClosestFace.Indices[i];
				if (Idx < Simplex.Size) {
					ContactPoint += Simplex.MinkowskiPoints[Idx];
				}
			}
			ContactPoint *= (1.0f / 3.0f);
			Result.Point = ContactPoint;

			return Result;
		}

		// 다면체 확장
		ExpandPolyhedron(Faces, NewPoint, Simplex, ShapeA, TransformA, ShapeB, TransformB);

		// 새로운 가장 가까운 면 찾기
		ClosestFaceIndex = FindClosestFace(Faces);
		if (ClosestFaceIndex < 0) {
			LOG_FUNC_CALL("[WARNING] No valid faces after expansion");
			break;
		}
	}

	// 최대 반복 횟수 도달 - 현재까지의 최선의 결과 반환
	if (!Faces.empty()) {
		FFace& ClosestFace = Faces[ClosestFaceIndex];
		Result.Normal = ClosestFace.Normal;
		Result.PenetrationDepth = ClosestFace.Distance;

		// 충돌 지점 계산
		Vector3 ContactPoint = Vector3::Zero;
		for (int i = 0; i < 3; ++i) {
			int Idx = ClosestFace.Indices[i];
			if (Idx < Simplex.Size) {
				ContactPoint = ContactPoint + Simplex.MinkowskiPoints[Idx];
			}
		}
		ContactPoint *= (1.0f / 3.0f);
		Result.Point = ContactPoint;
	}
	else
	{
		Result.bCollided = false;
	}

	return Result;
}

std::vector<FCollisionDetector::FFace> FCollisionDetector::BuildInitialPolyhedron(const FSimplex& Simplex)
{
	std::vector<FFace> Faces;
	if (Simplex.Size != 4) {
		LOG_FUNC_CALL("[ERROR] Simplex must have 4 points for polyhedron");
		return Faces;
	}

	// 사면체의 각 면 구성
	int FaceIndices[4][3] = {
		{0, 1, 2}, // ABC
		{0, 3, 1}, // ADB
		{0, 2, 3}, // ACD
		{1, 3, 2}  // BDC
	};

	// 각 면에 대한 정보 계산 및 저장
	for (int i = 0; i < 4; ++i) {
		FFace Face;
		for (int j = 0; j < 3; ++j) {
			Face.Indices[j] = FaceIndices[i][j];
		}

		// 면의 법선 계산 (외부 방향)
		XMVECTOR A = XMLoadFloat3(&Simplex.Points[Face.Indices[0]]);
		XMVECTOR B = XMLoadFloat3(&Simplex.Points[Face.Indices[1]]);
		XMVECTOR C = XMLoadFloat3(&Simplex.Points[Face.Indices[2]]);

		XMVECTOR AB = XMVectorSubtract(B, A);
		XMVECTOR AC = XMVectorSubtract(C, A);
		XMVECTOR Normal = XMVector3Cross(AB, AC);
		if (XMVectorGetX(XMVector3LengthSq(Normal)) < KINDA_SMALL) {
			LOG_FUNC_CALL("[WARNING] Degenerate face skipped");
			continue;
		}

		// 원점 방향 검사 - 법선이 항상 원점을 향하도록
		XMVECTOR ToOrigin = XMVectorNegate(A); // A에서 원점으로의 벡터

		if (XMVectorGetX(XMVector3Dot(Normal, ToOrigin)) < 0.0f) {
			// 법선이 원점과 반대 방향 - 인덱스 순서 변경
			std::swap(Face.Indices[1], Face.Indices[2]);

			// 법선 재계산
			B = XMLoadFloat3(&Simplex.Points[Face.Indices[1]]);
			C = XMLoadFloat3(&Simplex.Points[Face.Indices[2]]);
			AB = XMVectorSubtract(B, A);
			AC = XMVectorSubtract(C, A);
			Normal = XMVector3Cross(AB, AC);
		}

		// 법선 정규화
		Normal = XMVector3Normalize(Normal);

		// 면에서 원점까지의 거리 계산
		// 항상 양의 거리 사용 (법선은 이미 원점 방향)
		Face.Distance = std::abs(XMVectorGetX(XMVector3Dot(A, Normal)));

		XMStoreFloat3(&Face.Normal, Normal);

		Faces.push_back(Face);
	}

	return Faces;
}

int FCollisionDetector::FindClosestFace(const std::vector<FFace>& Faces)
{
	if (Faces.empty()) return -1;

	int ClosestIndex = 0;
	float MinDistance = Faces[0].Distance;

	for (size_t i = 1; i < Faces.size(); ++i) {
		if (Faces[i].Distance < MinDistance) {
			MinDistance = Faces[i].Distance;
			ClosestIndex = static_cast<int>(i);
		}
	}

	return ClosestIndex;
}

void FCollisionDetector::ExpandPolyhedron(
	std::vector<FFace>& Faces,
	const Vector3& NewPoint,
	FSimplex& Simplex,
	const ICollisionShape& ShapeA, const FTransform& TransformA,
	const ICollisionShape& ShapeB, const FTransform& TransformB)
{
	std::vector<bool> VisibleFaces(Faces.size(), false);

	// 새 점에서 볼 수 있는 면들 식별
	for (size_t i = 0; i < Faces.size(); ++i) {
		XMVECTOR vNormal = XMLoadFloat3(&Faces[i].Normal);
		XMVECTOR vNewPoint = XMLoadFloat3(&NewPoint);

		float Dot = XMVectorGetX(XMVector3Dot(vNormal, vNewPoint)) - Faces[i].Distance;

		// 새 점에서 면이 보이는지 확인 (법선 방향에서 바라봄)
		if (Dot > 0) {
			VisibleFaces[i] = true;
		}
	}

	// 새 점에서 보이는 면들의 경계 에지 수집
	std::vector<std::pair<int, int>> UniqueEdges;

	for (size_t i = 0; i < Faces.size(); ++i) {
		if (VisibleFaces[i]) {
			// 이 면의 모든 에지
			for (int j = 0; j < 3; ++j) {
				int EdgeStart = Faces[i].Indices[j];
				int EdgeEnd = Faces[i].Indices[(j + 1) % 3];

				// 에지가 지평선 에지인지 확인 (한 면만 보이는 에지)
				bool IsHorizonEdge = true;

				for (size_t k = 0; k < Faces.size(); ++k) {
					if (k != i && VisibleFaces[k]) {
						// 다른 보이는 면에서 이 에지를 공유하는지 확인
						for (int l = 0; l < 3; ++l) {
							int OtherStart = Faces[k].Indices[l];
							int OtherEnd = Faces[k].Indices[(l + 1) % 3];

							if ((EdgeStart == OtherEnd && EdgeEnd == OtherStart) ||
								(EdgeStart == OtherStart && EdgeEnd == OtherEnd)) {
								IsHorizonEdge = false;
								break;
							}
						}

						if (!IsHorizonEdge) break;
					}
				}

				if (IsHorizonEdge) {
					UniqueEdges.push_back({ EdgeStart, EdgeEnd });
				}
			}
		}
	}

	// 보이는 면 제거
	std::vector<FFace> NewFaces;
	for (size_t i = 0; i < Faces.size(); ++i) {
		if (!VisibleFaces[i]) {
			NewFaces.push_back(Faces[i]);
		}
	}

	// 새 점과 지평선 에지로 새 면 생성
	for (const auto& Edge : UniqueEdges) {
		FFace NewFace;
		NewFace.Indices[0] = Edge.first;
		NewFace.Indices[1] = Edge.second;
		NewFace.Indices[2] = Simplex.Size; // 새 점 인덱스

		// 새 면의 법선 계산
		XMVECTOR A = XMLoadFloat3(&Simplex.Points[NewFace.Indices[0]]);
		XMVECTOR B = XMLoadFloat3(&Simplex.Points[NewFace.Indices[1]]);
		XMVECTOR C = XMLoadFloat3(&NewPoint);

		XMVECTOR AB = XMVectorSubtract(B, A);
		XMVECTOR AC = XMVectorSubtract(C, A);
		XMVECTOR Normal = XMVector3Normalize(XMVector3Cross(AB, AC));

		// 법선이 원점을 향하는지 확인
		XMVECTOR ToOrigin = XMVectorNegate(A);
		if (XMVectorGetX(XMVector3Dot(Normal, ToOrigin)) < 0) {
			std::swap(NewFace.Indices[1], NewFace.Indices[2]);
			Normal = XMVectorNegate(Normal);
		}

		// 면의 거리 계산
		NewFace.Distance = XMVectorGetX(XMVector3Dot(A, Normal));
		XMStoreFloat3(&NewFace.Normal, Normal);

		NewFaces.push_back(NewFace);
	}

	// 새 점을 단체에 추가
	if (Simplex.Size < 16) { // 적절한 최대 크기 제한
		Simplex.Points[Simplex.Size] = NewPoint;

		// 민코프스키 차 공간의 점도 저장 (디버깅 및 충돌 점 계산용)
		Vector3 SupportA = ShapeA.GetSupportPoint(NewPoint, TransformA);
		Vector3 SupportB = ShapeB.GetSupportPoint(-NewPoint, TransformB);
		Simplex.MinkowskiPoints[Simplex.Size] = SupportA - SupportB;

		Simplex.Size++;
	}

	// 업데이트된 면 목록으로 교체
	Faces = std::move(NewFaces);
}