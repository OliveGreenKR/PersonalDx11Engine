#pragma once
#include "Math.h"
#include "Transform.h"
#include "CollisionDefines.h"
#include "CollisionShapeInterface.h"

using namespace DirectX;

// 충돌 검사 알고리즘 모음
class FCollisionDetector
{
    // 원점을 포함하는 Simplex 정보 (SIMD 최적화)
    struct alignas(16) FSimplex
    {
        XMVECTOR Points[4];        // 심플렉스 정점
        XMVECTOR SupportPointsA[4]; // 도형 A의 지원점
        XMVECTOR SupportPointsB[4]; // 도형 B의 지원점
        int32_t Size;              // 현재 심플렉스의 점 개수
    };

    // 다면체 구조: 정점과 면 인덱스
    struct alignas(16) FFace
    {
        int32_t Indices[3]; // 삼각형 정점 인덱스
        XMVECTOR Normal;    // 법선 (B->A 방향)
        float Distance;     // 원점에서 면까지의 거리
    };

public:
    FCollisionDetector();

public:
    // 이산 충돌 감지 (외부 인터페이스, 시그니처 변경 불가)
    FCollisionDetectionResult DetectCollisionDiscrete(
        const ICollisionShape& ShapeA,
        const FTransform& TransformA,
        const ICollisionShape& ShapeB,
        const FTransform& TransformB);

    // 연속 충돌 감지 (외부 인터페이스, 시그니처 변경 불가)
    FCollisionDetectionResult DetectCollisionCCD(
        const ICollisionShape& ShapeA,
        const FTransform& PrevTransformA,
        const FTransform& CurrentTransformA,
        const ICollisionShape& ShapeB,
        const FTransform& PrevTransformB,
        const FTransform& CurrentTransformB,
        const float DeltaTime);

private:
    // 형상 기반 이산 충돌 검사 (기존 유지)
    FCollisionDetectionResult DetectCollisionShapeBasedDiscrete(
        const ICollisionShape& ShapeA,
        const FTransform& TransformA,
        const ICollisionShape& ShapeB,
        const FTransform& TransformB);

    // GJK+EPA 통합 충돌 감지
    FCollisionDetectionResult DetectCollisionGJKEPA(
        const ICollisionShape& ShapeA,
        const FTransform& TransformA,
        const ICollisionShape& ShapeB,
        const FTransform& TransformB);

    // GJK 알고리즘: 충돌 여부 및 Simplex 생성
    bool GJKCollision(
        const ICollisionShape& ShapeA,
        const FTransform& TransformA,
        const ICollisionShape& ShapeB,
        const FTransform& TransformB,
        FSimplex& OutSimplex);

    // EPA 알고리즘: 침투 깊이와 충돌 정보 계산
    FCollisionDetectionResult EPACollision(
        const ICollisionShape& ShapeA,
        const FTransform& TransformA,
        const ICollisionShape& ShapeB,
        const FTransform& TransformB,
        const FSimplex& Simplex);

    void UpdateFaceNormalsAndDistances(const std::vector<XMVECTOR>& Polytope, const std::vector<size_t>& Faces, 
                                       std::vector<XMVECTOR>& Normals, std::vector<float>& Distances, const std::vector<XMVECTOR>& SupportA,
                                       const std::vector<XMVECTOR>& SupportB, 
                                       float& MinDistance, XMVECTOR& MinNormal);

    XMVECTOR ComputeCollisionPoint(const std::vector<size_t>& Faces, const std::vector<XMVECTOR>& Normals,
                                   XMVECTOR MinNormal,
                                   const std::vector<XMVECTOR>& Polytope, 
                                   const std::vector<XMVECTOR>& SupportA, const std::vector<XMVECTOR>& SupportB);

    void AddIfUniqueEdge(std::vector<std::pair<size_t, size_t>>& Edges,
                         const std::vector<size_t>& Faces, 
                         size_t A, size_t B);

    // Minkowski 차분 지원점 계산 (SIMD 최적화)
    XMVECTOR ComputeMinkowskiSupport(
        const ICollisionShape& ShapeA,
        const FTransform& TransformA,
        const ICollisionShape& ShapeB,
        const FTransform& TransformB,
        const XMVECTOR& Direction,
        XMVECTOR& OutSupportA,
        XMVECTOR& OutSupportB);

    // Simplex 업데이트 헬퍼 함수
    bool UpdateSimplex(FSimplex& Simplex, XMVECTOR& Direction);

#pragma region Shape-Based Helper
    // Box-Box 충돌 검사 (AABB)
    FCollisionDetectionResult BoxBoxAABB(
        const Vector3& ExtentA, const FTransform& TransformA,
        const Vector3& ExtentB, const FTransform& TransformB);

    // Box-Box 충돌 검사 (SAT)
    FCollisionDetectionResult BoxBoxSAT(
        const Vector3& ExtentA, const FTransform& TransformA,
        const Vector3& ExtentB, const FTransform& TransformB);

    // Sphere-Sphere 충돌 검사
    FCollisionDetectionResult SphereSphere(
        float RadiusA, const FTransform& TransformA,
        float RadiusB, const FTransform& TransformB);

    // Box-Sphere 충돌 검사
    FCollisionDetectionResult BoxSphereSimple(
        const Vector3& BoxExtent, const FTransform& TransformA,
        float SphereRadius, const FTransform& SphereTransform);
#pragma endregion

#pragma region Conservative Advancement Helper
    // 두 충돌체 간의 최소 거리 계산
    float ComputeMinimumDistance(
        const ICollisionShape& ShapeA,
        const FTransform& TransformA,
        const ICollisionShape& ShapeB,
        const FTransform& TransformB) const;

    // 두 충돌체 간의 상대 속도 계산
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
    float CCDTimeStep = 0.02f;         // CCD 시간 스텝
    float CASafetyFactor = 1.1f;       // CA 안전 계수
    float CAMinTimeStep = 0.001f;      // CA 최소 시간 스텝
    int CCDMaxIterations = 10;         // CCD 최대 반복 횟수
    float DistanceThreshold = 0.001f;  // 접촉 간주 거리 임계값
    int MaxGJKIterations = 20;         // GJK 최대 반복 횟수
    int MaxEPAIterations = 20;         // EPA 최대 반복 횟수
    float EPATolerance = 0.001f;       // EPA 수렴 임계값

    bool bUseGJKEPA = true;
};
