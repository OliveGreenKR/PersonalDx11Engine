#pragma once
#include "Math.h"
#include "Transform.h"
#include "CollisionDefines.h"


// 충돌 검사 알고리즘 모음
class  FCollisionDetector
{
public:
    // 이산 충돌 감지
    FCollisionDetectionResult DetectCollisionDiscrete(
        const FCollisionShapeData& ShapeA,
        const FTransform& TransformA,
        const FCollisionShapeData& ShapeB,
        const FTransform& TransformB);
     // 연속(Continuous) 충돌 감지
    FCollisionDetectionResult DetectCollisionCCD(
        const FCollisionShapeData& ShapeA,
        const FTransform& PrevTransformA,
        const FTransform& CurrentTransformA,
        const FCollisionShapeData& ShapeB,
        const FTransform& PrevTransformB,
        const FTransform& CurrentTransformB,
        const float DeltaTime);

private:
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

public:
    float TimeStep = 0.02f;
}; 