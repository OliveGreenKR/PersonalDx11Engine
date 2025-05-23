#pragma once
#include "Math.h"
#include "Transform.h"
#include "CollisionDefines.h"
#include "CollisionShapeInterface.h"

using namespace DirectX;

struct FAABB;

// 충돌 검사 알고리즘 모음
class FCollisionDetector
{
//for test
public:
    // 원점을 포함하는 Simplex 정보 (SIMD 최적화)
    struct alignas(16) FSimplex
    {
        XMVECTOR Points[4];        // 심플렉스 정점
        XMVECTOR SupportPointsA[4]; // 도형 A의 지원점
        XMVECTOR SupportPointsB[4]; // 도형 B의 지원점
        int32_t Size;              // 현재 심플렉스의 점 개수
    };

    struct Edge {
        int Start, End;

        Edge(int start, int end) : Start(start), End(end) {
            if (start > end) std::swap(Start, End); // 순서에 상관없이 동일한 에지로 처리
        }

        bool operator==(const Edge& other) const {
            return Start == other.Start && End == other.End;
        }
    };

    struct EdgeHash {
        std::size_t operator()(const Edge& e) const {
            return std::hash<int>()(e.Start) ^ std::hash<int>()(e.End);
        }
    };

    struct PolytopeSOA {
        std::vector<XMVECTOR> Vertices;
        std::vector<XMVECTOR> VerticesA; // Corresponding points on ShapeA for Poly.Vertices
        std::vector<XMVECTOR> VerticesB; // Corresponding points on ShapeB for Poly.Vertices
        std::vector<XMVECTOR> Normals;
        std::vector<float> Distances;
        std::vector<int> Indices; // 각 면의 정점 인덱스 (3개씩 묶음)
    };

public:
    FCollisionDetector();

public:
    // 이산 충돌 감지 (외부 인터페이스, 시그니처 변경 불가)
    FCollisionDetectionResult DetectCollisionDiscrete(const ICollisionShape& ShapeA, const FTransform& WorldTransformA,
                                                      const ICollisionShape& ShapeB, const FTransform& WroldTransformB);

    // 연속 충돌 감지 (외부 인터페이스, 시그니처 변경 불가)
    FCollisionDetectionResult DetectCollisionCCD(
        const ICollisionShape& ShapeA,
        const FTransform& PrevWorldTransformA,
        const FTransform& CurrentWorldTransformA,
        const ICollisionShape& ShapeB,
        const FTransform& PrevWorldTransformB,
        const FTransform& CurrentWorldTransformB,
        const float DeltaTime);

public:
    // 형상 기반 이산 충돌 검사 (기존 유지)
    FCollisionDetectionResult DetectCollisionShapeBasedDiscrete(
        const ICollisionShape& ShapeA,
        const FTransform& WorldTransformA,
        const ICollisionShape& ShapeB,
        const FTransform& WorldTransformB);

    // GJK+EPA 통합 충돌 감지
    FCollisionDetectionResult DetectCollisionGJKEPA(
        const ICollisionShape& ShapeA,
        const FTransform& WorldTransformA,
        const ICollisionShape& ShapeB,
        const FTransform& WorldTransformB);

    // GJK 알고리즘: 충돌 여부 및 Simplex 생성
    bool GJKCollision(
        const ICollisionShape& ShapeA,
        const FTransform& WorldTransformA,
        const ICollisionShape& ShapeB,
        const FTransform& WorldTransformB,
        FSimplex& OutSimplex);

    // EPA 알고리즘: 침투 깊이와 충돌 정보 계산
    FCollisionDetectionResult EPACollision(
        const ICollisionShape& ShapeA,
        const FTransform& WorldTransformA,
        const ICollisionShape& ShapeB,
        const FTransform& WorldTransformB,
        const FSimplex& Simplex);

    // Minkowski 차분 지원점 계산 (SIMD 최적화)
    XMVECTOR ComputeMinkowskiSupport(
        const ICollisionShape& ShapeA,
        const FTransform& WorldTransformA,
        const ICollisionShape& ShapeB,
        const FTransform& WorldTransformB,
        const XMVECTOR& Direction,
        XMVECTOR& OutSupportA,
        XMVECTOR& OutSupportB);

#pragma region GJKEPA Helper
public: //for test 'public'
    // Simplex 업데이트 헬퍼 함수
    bool UpdateSimplex(FSimplex& Simplex, XMVECTOR& Direction);

    XMVECTOR ClosestPointOnSegmentToOrigin(FXMVECTOR P1, FXMVECTOR P2);

    bool GJKHandleTetrahedron(FSimplex& Simplex, XMVECTOR& Direction);

    void UpdatePolytope(PolytopeSOA& Poly, int NewPointIndex);

    void CalculateFaceNormalAndDistance(const PolytopeSOA& Poly, 
                                        int face_index,
                                        DirectX::XMVECTOR& out_normal, 
                                        float& out_distance) const;

    bool IsFaceVisible(const PolytopeSOA& Poly, int face_index, const DirectX::XMVECTOR& new_point_vector,
                       const DirectX::XMVECTOR& face_normal,
                       float face_distance) const;

    std::vector<std::pair<int, int>> BuildHorizonEdges(const PolytopeSOA& Poly, 
                                                       const std::vector<int>& visible_face_indices) const;

    void CreateNewFaces(const std::vector<std::pair<int, int>>& horizon_edges, 
                        int new_point_index, 
                        const PolytopeSOA& original_poly,
                        std::vector<int>& new_indices, 
                        std::vector<DirectX::XMVECTOR>& new_normals, 
                        std::vector<float>& new_distances) const;

    void DrawPolytope(const FCollisionDetector::PolytopeSOA& Polytope, 
                      float LifeTime, 
                      bool bDrawNormals, const Vector4 & Color);
#pragma endregion
#pragma region Shape-Based Helper
private:
    // Box-Box 충돌 검사 (AABB)
    FCollisionDetectionResult BoxBoxAABB(
        const Vector3& ExtentA, const FTransform& WorldTransformA,
        const Vector3& ExtentB, const FTransform& WorldTransformB);

    // Box-Box 충돌 검사 (SAT)
    FCollisionDetectionResult BoxBoxSAT(
        const Vector3& ExtentA, const FTransform& WorldTransformA,
        const Vector3& ExtentB, const FTransform& WorldTransformB);

    // Sphere-Sphere 충돌 검사
    FCollisionDetectionResult SphereSphere(
        float RadiusA, const FTransform& WorldTransformA,
        float RadiusB, const FTransform& WorldTransformB);

    // Box-Sphere 충돌 검사
    FCollisionDetectionResult BoxSphereSimple(
        const Vector3& BoxExtent, const FTransform& WorldTransformA,
        float SphereRadius, const FTransform& SphereTransform);
#pragma endregion
#pragma region Shape_Based SweptVolume
private:
    FAABB CalculateSweptAABB(const ICollisionShape& InShape, const FTransform& PrevTransform, 
                             const FTransform& CurrentTransform);
    FAABB CalculateWorldAABB(const ICollisionShape& InShape, const FTransform& InWorldTransform);
#pragma endregion

public:
    float CCDTimeStep = 0.02f;         // CCD 시간 스텝
    float CASafetyFactor = 1.1f;       // CA 안전 계수
    float CAMinTimeStep = 0.001f;      // CA 최소 시간 스텝
    int MaxCCDIterations = 10;         // CCD 최대 반복 횟수
    float DistanceThreshold = 0.001f;  // 접촉 간주 거리 임계값
    int MaxGJKIterations = 20;         // GJK 최대 반복 횟수
    int MaxEPAIterations = 20;         // EPA 최대 반복 횟수
    float EPATolerance = 0.001f;       // EPA 수렴 임계값

    bool bUseGJKEPA = true;
};
