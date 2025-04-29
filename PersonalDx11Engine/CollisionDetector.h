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
#pragma region Shape_based Helper
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
#pragma endregion
#pragma region Conservative Advanced Helper
private:
    // 두 충돌체 간의 최소 거리 계산
    float ComputeMinimumDistance(
        const ICollisionShape& ShapeA,
        const FTransform& TransformA,
        const ICollisionShape& ShapeB,
        const FTransform& TransformB) const;

    // 두 충돌체 간의 상대 속도 계산 (선형 + 회전)
    Vector3 ComputeRelativeVelocity(
        const FTransform& PrevTransformA,
        const FTransform& CurrentTransformA,
        const FTransform& PrevTransformB,
        const FTransform& CurrentTransformB,
        const Vector3& ContactPoint,
        float DeltaTime) const;

    // 특정 방향으로의 접근 속도 계산
    float ComputeApproachSpeed(
        const Vector3& RelativeVelocity,
        const Vector3& Direction) const;

    // 콘서버티브 어드밴스먼트 단계 수행
    float PerformConservativeAdvancementStep(
        const ICollisionShape& ShapeA,
        const FTransform& PrevTransformA,
        const FTransform& CurrentTransformA,
        const ICollisionShape& ShapeB,
        const FTransform& PrevTransformB,
        const FTransform& CurrentTransformB,
        float CurrentTime,
        float DeltaTime,
        Vector3& CollisionNormal) const;
#pragma endregion
public:
    float CCDTimeStep = 0.02f;
    float CASafetyFactor = 1.1f;         // 안전 계수
    float CAMinTimeStep = 0.001f;        // 최소 시간 스텝
    int   CCDMaxIterations = 10;          // 최대 반복 횟수

    float DistanceThreshold = 0.001f;  // 접촉 간주 거리 임계값
    int MAX_GJK_ITERATIONS = 5;

}; 