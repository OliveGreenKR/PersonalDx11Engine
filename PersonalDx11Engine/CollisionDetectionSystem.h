#pragma once
#include "Math.h"
#include "Transform.h"
#include "CollisionDefines.h"


// 충돌 검사 알고리즘 모음
class  UCollisionDetectionSystem
{
public:
     // 연속(Continuous) 충돌 감지
    static FCollisionDetectionResult DetectCollisionCCD(
        const FCollisionShapeData& ShapeA,
        const FTransform& PrevTransformA,
        const FTransform& CurrentTransformA,
        const FCollisionShapeData& ShapeB,
        const FTransform& PrevTransformB,
        const FTransform& CurrentTransformB);

private:
    // Box-Box 충돌 검사
    static FCollisionDetectionResult BoxBoxAABB(
        const Vector3& ExtentA, const FTransform& TransformA,
        const Vector3& ExtentB, const FTransform& TransformB);

    static FCollisionDetectionResult BoxBoxSAT(
        const Vector3& ExtentA, const FTransform& TransformA,
        const Vector3& ExtentB, const FTransform& TransformB);

    // Sphere-Sphere 충돌 검사
    static FCollisionDetectionResult SphereSphere(
        float RadiusA, const FTransform& TransformA,
        float RadiusB, const FTransform& TransformB);

    // Box-Sphere 충돌 검사
    static FCollisionDetectionResult BoxSphereSimple(
        const Vector3& BoxExtent, const FTransform& BoxTransform,
        float SphereRadius, const FTransform& SphereTransform);
}; 