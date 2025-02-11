#include "CollisionDetector.h"
#include <algorithm>

FCollisionDetectionResult FCollisionDetector::DetectCollisionDiscrete(
	const FCollisionShapeData& ShapeA,
	const FTransform& TransformA,
	const FCollisionShapeData& ShapeB,
	const FTransform& TransformB)
{
	// ���� Ÿ�Կ� ���� ������ �浹 �˻� �Լ� ȣ��
	if (ShapeA.Type == ECollisionShapeType::Sphere && ShapeB.Type == ECollisionShapeType::Sphere)
	{
		return SphereSphere(
			ShapeA.GetSphereRadius(), TransformA,
			ShapeB.GetSphereRadius(), TransformB);
	}
	else if (ShapeA.Type == ECollisionShapeType::Box && ShapeB.Type == ECollisionShapeType::Box)
	{
		return BoxBoxSAT(
			ShapeA.GetBoxHalfExtents(), TransformA,
			ShapeB.GetBoxHalfExtents(), TransformB);
	}
	else if (ShapeA.Type == ECollisionShapeType::Box && ShapeB.Type == ECollisionShapeType::Sphere)
	{
		return BoxSphereSimple(
			ShapeA.GetBoxHalfExtents(), TransformA,
			ShapeB.GetSphereRadius(), TransformB);
	}
	else if (ShapeA.Type == ECollisionShapeType::Sphere && ShapeB.Type == ECollisionShapeType::Box)
	{
		auto Result = BoxSphereSimple(
			ShapeB.GetBoxHalfExtents(), TransformB,
			ShapeA.GetSphereRadius(), TransformA);
		// ��� ���� ����
		Result.Normal = -Result.Normal;
		return Result;
	}

	return FCollisionDetectionResult();  // �������� �ʴ� ����
}

FCollisionDetectionResult FCollisionDetector::DetectCollisionCCD(
	const FCollisionShapeData& ShapeA,
	const FTransform& PrevTransformA, 
	const FTransform& CurrentTransformA, 
	const FCollisionShapeData& ShapeB, 
	const FTransform& PrevTransformB, 
	const FTransform& CurrentTransformB,
	const float DeltaTime)
{
	// �ʱ� ���¿� ���� ���¿����� �浹 �˻�
	FCollisionDetectionResult StartResult = DetectCollisionDiscrete(ShapeA, PrevTransformA, ShapeB, PrevTransformB);
	if (StartResult.bCollided)
	{
		StartResult.TimeOfImpact = 0.0f;  // ���ۺ��� �浹 ����
		return StartResult;
	}

	FCollisionDetectionResult EndResult = DetectCollisionDiscrete(ShapeA, CurrentTransformA, ShapeB, CurrentTransformB);
	if (!EndResult.bCollided)
	{
		// ���۰� �� ��� �浹���� ������, ��ΰ� �ſ� ª�� ��� -> �浹 �˻� �ǳʶ��
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

		// �߰� ������ Transform ���
		FTransform InterpolatedTransformA = FTransform::InterpolateTransform(PrevTransformA, CurrentTransformA, alpha);
		FTransform InterpolatedTransformB = FTransform::InterpolateTransform(PrevTransformB, CurrentTransformB, alpha);

		//���� �浹 ���� ã��
		Result = DetectCollisionDiscrete(ShapeA, InterpolatedTransformA, ShapeB, InterpolatedTransformB);

		if (Result.bCollided)
		{
			Result.TimeOfImpact = currentTime;
			break;
		}

		currentTime += TimeStep;
	}

	return EndResult;  // ���� ������ ��� ��ȯ
}

FCollisionDetectionResult FCollisionDetector::SphereSphere(
	float RadiusA, const FTransform& TransformA,
	float RadiusB, const FTransform& TransformB)
{
	FCollisionDetectionResult Result;

	// SIMD ������ ���� ���� �ε�
	XMVECTOR vPosA = XMLoadFloat3(&TransformA.Position);
	XMVECTOR vPosB = XMLoadFloat3(&TransformB.Position);

	// �߽��� ���� ���� ���
	XMVECTOR vDelta = XMVectorSubtract(vPosB, vPosA);
	float distanceSquared = XMVectorGetX(XMVector3LengthSq(vDelta));

	float radiusSum = RadiusA + RadiusB;
	float penetrationDepth = radiusSum - sqrt(distanceSquared);

	// �浹 �˻�
	if (penetrationDepth <= 0.0f)
	{
		return Result;  // �⺻�� ��ȯ (�浹 ����)
	}

	Result.bCollided = true;

	// �浹 ���� ����
	float distance = sqrt(distanceSquared);
	if (distance < KINDA_SMALL)
	{
		// ��ü�� ���� ���� ��ġ�� �ִ� ���
		Result.Normal = Vector3::Up;  // �⺻ ���� ����
		Result.PenetrationDepth = radiusSum;
		XMStoreFloat3(&Result.Point, vPosA);
	}
	else
	{
		// ����ȭ�� �浹 ���� ���
		XMVECTOR vNormal = XMVectorDivide(vDelta, XMVectorReplicate(distance));
		XMStoreFloat3(&Result.Normal, vNormal);
		Result.PenetrationDepth = penetrationDepth;

		// �浹 ������ �� ���� �߽��� �մ� ���󿡼� ù ��° ���� ǥ�鿡 ��ġ
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

	// ���� ���������� �ڽ� �ּ�/�ִ��� ���
	Vector3 MinA = TransformA.Position - ExtentA;
	Vector3 MaxA = TransformA.Position + ExtentA;
	Vector3 MinB = TransformB.Position - ExtentB;
	Vector3 MaxB = TransformB.Position + ExtentB;

	// AABB �浹 �˻�
	if (MinA.x <= MaxB.x && MaxA.x >= MinB.x &&
		MinA.y <= MaxB.y && MaxA.y >= MinB.y &&
		MinA.z <= MaxB.z && MaxA.z >= MinB.z)
	{
		Result.bCollided = true;

		// �� ���� ��ħ(ħ��) ���� ���
		Vector3 PenetrationVec;
		PenetrationVec.x = std::min(MaxA.x - MinB.x, MaxB.x - MinA.x);
		PenetrationVec.y = std::min(MaxA.y - MinB.y, MaxB.y - MinA.y);
		PenetrationVec.z = std::min(MaxA.z - MinB.z, MaxB.z - MinA.z);

		// ���� ���� ħ�� ������ �浹 �������� ���
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

		// �浹 ���� ����
		switch (minIndex)
		{
			case 0: Result.Normal.x = (TransformA.Position.x < TransformB.Position.x) ? -1.0f : 1.0f; break;
			case 1: Result.Normal.y = (TransformA.Position.y < TransformB.Position.y) ? -1.0f : 1.0f; break;
			case 2: Result.Normal.z = (TransformA.Position.z < TransformB.Position.z) ? -1.0f : 1.0f; break;
		}

		// �浹 ������ �� �ڽ��� �߽��� ������ �߰������� ����
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

	// �� �ڽ��� �� ���� ���� ����
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

	// SAT �˻縦 ���� �� ���
	XMVECTOR vSeparatingAxes[15];
	int axisIndex = 0;

	// �ڽ� A�� ��
	for (int i = 0; i < 3; i++)
		vSeparatingAxes[axisIndex++] = vAxesA[i];

	// �ڽ� B�� ��
	for (int i = 0; i < 3; i++)
		vSeparatingAxes[axisIndex++] = vAxesB[i];

	// �� ���� ����
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			vSeparatingAxes[axisIndex++] = XMVector3Cross(vAxesA[i], vAxesB[j]);
		}
	}

	float minPenetration = FLT_MAX;
	XMVECTOR vCollisionNormal = XMVectorZero();

	// �� �࿡ ���� ���� �˻�
	for (int i = 0; i < 15; i++)
	{
		XMVECTOR vLengthSq = XMVector3LengthSq(vSeparatingAxes[i]);
		if (XMVectorGetX(vLengthSq) < KINDA_SMALL)
			continue;

		XMVECTOR vAxis = XMVector3Normalize(vSeparatingAxes[i]);

		// �� �ڽ��� ���� �ݰ� ���
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
			return Result;  // �и��� �߰�

		if (penetration < minPenetration)
		{
			minPenetration = penetration;
			float direction = XMVectorGetX(XMVector3Dot(vDelta, vAxis));
			vCollisionNormal = direction >= 0 ? vAxis : XMVectorNegate(vAxis);
		}
	}

	// �浹 �߻�
	Result.bCollided = true;
	XMStoreFloat3(&Result.Normal, vCollisionNormal);
	Result.PenetrationDepth = minPenetration;

	// �浹 ���� ���
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

	// 1. �ڽ��� ���� �������� ������ �̵�
	// �ڽ��� ȸ���� ����ϱ� ���� ��ȸ�� ��� ���
	XMVECTOR vSpherePos = XMLoadFloat3(&SphereTransform.Position);
	XMVECTOR vBoxPos = XMLoadFloat3(&BoxTransform.Position);
	Matrix BoxRotationInv = XMMatrixTranspose(BoxTransform.GetRotationMatrix());

	// ���� �߽��� ȸ��
	XMVECTOR vRelativePos = XMVectorSubtract(vSpherePos, vBoxPos);  // ��� ��ġ
	XMVECTOR vLocalSpherePos = XMVector3Transform(vRelativePos, BoxRotationInv);

	// 2. �ڽ��� ���� �������� ���� �߽����� ���� ����� �� ã��
	Vector3 localSphere;
	XMStoreFloat3(&localSphere, vLocalSpherePos);

	Vector3 closestPoint;
	// �� �ະ�� �ڽ��� ������ Ŭ����
	closestPoint.x = Math::Clamp(localSphere.x, -BoxExtent.x, BoxExtent.x);
	closestPoint.y = Math::Clamp(localSphere.y, -BoxExtent.y, BoxExtent.y);
	closestPoint.z = Math::Clamp(localSphere.z, -BoxExtent.z, BoxExtent.z);

	// 3. ���� �߽����� ���� ����� �� ������ �Ÿ� ���
	XMVECTOR vClosestPoint = XMLoadFloat3(&closestPoint);
	XMVECTOR vDelta = XMVectorSubtract(vLocalSpherePos, vClosestPoint);
	float distanceSquared = XMVectorGetX(XMVector3LengthSq(vDelta));

	// �浹 �˻�: �Ÿ��� ���� ���������� ũ�� �浹���� ����
	if (distanceSquared > SphereRadius * SphereRadius)
		return Result;  // �浹 ����

	// 4. �浹 ���� ���
	Result.bCollided = true;
	float distance = sqrt(distanceSquared);

	// ���� �߽��� �ڽ��� ���� ��ġ�ϴ� ���
	if (distance < KINDA_SMALL)
	{
		// �ּ� ħ�� ���� ã��
		float penetrations[6] = {
			BoxExtent.x + localSphere.x,  // -x ����
			BoxExtent.x - localSphere.x,  // +x ����
			BoxExtent.y + localSphere.y,  // -y ����
			BoxExtent.y - localSphere.y,  // +y ����
			BoxExtent.z + localSphere.z,  // -z ����
			BoxExtent.z - localSphere.z   // +z ����
		};

		// ���� ���� ħ�� ���� ã��
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

		// �ּ� ħ�� �������� ���� ����
		Vector3 localNormal = Vector3::Zero;
		switch (minIndex)
		{
			case 0: localNormal.x = -1.0f; break;  // -x ����
			case 1: localNormal.x = 1.0f; break;   // +x ����
			case 2: localNormal.y = -1.0f; break;  // -y ����
			case 3: localNormal.y = 1.0f; break;   // +y ����
			case 4: localNormal.z = -1.0f; break;  // -z ����
			case 5: localNormal.z = 1.0f; break;   // +z ����
		}

		XMVECTOR vNormal = XMLoadFloat3(&localNormal);
		Result.PenetrationDepth = minPenetration + SphereRadius;

		// ������ ���� �������� ��ȯ
		Matrix BoxRotation = BoxTransform.GetRotationMatrix();
		vNormal = XMVector3Transform(vNormal, BoxRotation);
		XMStoreFloat3(&Result.Normal, vNormal);

		// �浹 ������ �ڽ��� ǥ��
		XMVECTOR vWorldClosestPoint = XMVector3Transform(vClosestPoint, BoxRotation);
		vWorldClosestPoint = XMVectorAdd(vWorldClosestPoint, vBoxPos);
		XMStoreFloat3(&Result.Point, vWorldClosestPoint);
	}
	// ���� �ڽ� �ܺο� �ִ� �Ϲ����� ���
	else
	{
		// ���� ���������� ���� ���͸� �������� ���
		XMVECTOR vNormal = XMVector3Normalize(vDelta);
		Result.PenetrationDepth = SphereRadius - distance;

		// ������ ���� �������� ��ȯ
		Matrix BoxRotation = BoxTransform.GetRotationMatrix();
		vNormal = XMVector3Transform(vNormal, BoxRotation);
		XMStoreFloat3(&Result.Normal, vNormal);

		// �浹 ������ ���� ǥ�鿡�� ���� ���������� ����
		XMVECTOR vWorldClosestPoint = XMVector3Transform(vClosestPoint, BoxRotation);
		vWorldClosestPoint = XMVectorAdd(vWorldClosestPoint, vBoxPos);
		XMStoreFloat3(&Result.Point, vWorldClosestPoint);
	}

	return Result;
}
