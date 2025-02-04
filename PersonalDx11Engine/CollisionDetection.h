#pragma once
#include "Math.h"
#include "Transform.h"

// 순수한 충돌 검사 결과 데이터
struct FCollisionDetectionResult
{
    bool bCollided = false;
    Vector3 Normal = Vector3::Zero;
    Vector3 Point = Vector3::Zero;
    float PenetrationDepth = 0.0f;
    float TimeOfImpact = 0.0f;
};

// 충돌 검사 알고리즘 모음
namespace CollisionDetection
{
    // Box-Box 충돌 검사
    FCollisionDetectionResult BoxBoxAABB(
        const Vector3& ExtentA, const FTransform& TransformA,
        const Vector3& ExtentB, const FTransform& TransformB);

    FCollisionDetectionResult BoxBoxSAT(
        const Vector3& ExtentA, const FTransform& TransformA,
        const Vector3& ExtentB, const FTransform& TransformB);

    // Sphere-Sphere 충돌 검사
    FCollisionDetectionResult SphereSphere(
        float RadiusA, const FTransform& TransformA,
        float RadiusB, const FTransform& TransformB);

    // Box-Sphere 충돌 검사
    FCollisionDetectionResult BoxSphereSimple(
        const Vector3& BoxExtent, const FTransform& BoxTransform,
        float SphereRadius, const FTransform& SphereTransform);
}; 