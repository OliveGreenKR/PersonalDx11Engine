#pragma once
#include "Math.h"
#include "Transform.h"

// ������ �浹 �˻� ��� ������
struct FCollisionDetectionResult
{
    bool bCollided = false;
    Vector3 Normal = Vector3::Zero;
    Vector3 Point = Vector3::Zero;
    float PenetrationDepth = 0.0f;
    float TimeOfImpact = 0.0f;
};

// �浹 �˻� �˰��� ����
namespace CollisionDetection
{
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
}; 