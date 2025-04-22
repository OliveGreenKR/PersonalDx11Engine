#pragma once
#include "Math.h"
#include "Transform.h"
#include "CollisionDefines.h"
#include "CollisionShapeInterface.h"

using namespace DirectX;

// 충돌 검사 알고리즘 모음
class  FCollisionDetector
{
    // 원점을 포함하는 Simplex 정보
    struct alignas(16) FSimplex
    {
        XMVECTOR Points[4];        // 심플렉스 정점 (SIMD 최적화를 위해 XMVECTOR 사용)
        int32_t Size;              // 현재 심플렉스의 점 개수
    };

public:
    FCollisionDetector();

public:
    // 이산 충돌 감지
    FCollisionDetectionResult DetectCollisionDiscrete(
        const ICollisionShape& ShapeA,
        const FTransform& TransformA,
        const ICollisionShape& ShapeB,
        const FTransform& TransformB);
    // 연속(Continuous) 충돌 감지
    FCollisionDetectionResult DetectCollisionCCD(
        const ICollisionShape& ShapeA,
        const FTransform& PrevTransformA,
        const FTransform& CurrentTransformA,
        const ICollisionShape& ShapeB,
        const FTransform& PrevTransformB,
        const FTransform& CurrentTransformB,
        const float DeltaTime);

private:
    FCollisionDetectionResult DetectCollisionShapeBasedDiscrete(const ICollisionShape& ShapeA,const FTransform& TransformA,
                                                           const ICollisionShape& ShapeB,const FTransform& TransformB);
    //// TODO GJK+EPA 통합 충돌 감지 함수
    //FCollisionDetectionResult DetectCollisionGJKEPADiscrete(const ICollisionShape& ShapeA, const FTransform& TransformA,
    //                                                        const ICollisionShape& ShapeB, const FTransform& TransformB);

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
    float CCDTimeStep = 0.02f;
}; 