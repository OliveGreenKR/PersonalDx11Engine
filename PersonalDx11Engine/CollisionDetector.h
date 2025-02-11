#pragma once
#include "Math.h"
#include "Transform.h"
#include "CollisionDefines.h"


// �浹 �˻� �˰��� ����
class  FCollisionDetector
{
public:
    // �̻� �浹 ����
    FCollisionDetectionResult DetectCollisionDiscrete(
        const FCollisionShapeData& ShapeA,
        const FTransform& TransformA,
        const FCollisionShapeData& ShapeB,
        const FTransform& TransformB);
     // ����(Continuous) �浹 ����
    FCollisionDetectionResult DetectCollisionCCD(
        const FCollisionShapeData& ShapeA,
        const FTransform& PrevTransformA,
        const FTransform& CurrentTransformA,
        const FCollisionShapeData& ShapeB,
        const FTransform& PrevTransformB,
        const FTransform& CurrentTransformB,
        const float DeltaTime);

private:
    // Box-Box �浹 �˻�
    FCollisionDetectionResult BoxBoxAABB(
        const Vector3& ExtentA, const FTransform& TransformA,
        const Vector3& ExtentB, const FTransform& TransformB);

    FCollisionDetectionResult BoxBoxSAT(
        const Vector3& ExtentA, const FTransform& TransformA,
        const Vector3& ExtentB, const FTransform& TransformB);

    // Sphere-Sphere �浹 �˻�
    FCollisionDetectionResult SphereSphere(
        float RadiusA, const FTransform& TransformA,
        float RadiusB, const FTransform& TransformB);

    // Box-Sphere �浹 �˻�
     FCollisionDetectionResult BoxSphereSimple(
        const Vector3& BoxExtent, const FTransform& BoxTransform,
        float SphereRadius, const FTransform& SphereTransform);

public:
    float TimeStep = 0.02f;
}; 