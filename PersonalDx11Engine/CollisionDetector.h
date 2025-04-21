#pragma once
#include "Math.h"
#include "Transform.h"
#include "CollisionDefines.h"
#include "CollisionShapeInterface.h"

using namespace DirectX;

// 충돌 검사 알고리즘 모음
class  FCollisionDetector
{
    //원점을 포함하는 Simplex 정보
    struct FSimplex
    {
        XMVECTOR Points[4];          // 심플렉스 정점
        int Size;                   // 현재 심플렉스의 점 개수
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
    // GJK+EPA 통합 충돌 감지 함수
    FCollisionDetectionResult DetectCollisionGJKEPADiscrete(const ICollisionShape& ShapeA, const FTransform& TransformA,
                                                            const ICollisionShape& ShapeB, const FTransform& TransformB);

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

private:
    bool GJK(const ICollisionShape& ShapeA, const FTransform& TransformA,
             const ICollisionShape& ShapeB, const FTransform& TransformB,
             FSimplex& InSimplex);
    // 서포트 함수 - 특정 방향으로 가장 멀리 있는 점 반환
    XMVECTOR Support(const ICollisionShape& ShapeA, const ICollisionShape& ShapeB,
                     const Vector3 & Direction);

    bool ProcessSimplex(FSimplex& Simplex, XMVECTOR& Direction);
    bool ProcessLine(FSimplex& Simplex, XMVECTOR& Direction);
    bool ProcessTriangle(FSimplex& Simplex, XMVECTOR& Direction);
    bool ProcessTetrahedron(FSimplex& Simplex, XMVECTOR& Direction);


public:
    float CCDTimeStep = 0.02f;
    int GJK_MAX_ITERATION = 32;
    float GJK_EPSILON = 0.0001f;
    float EPA_EPSILON = 0.0001f;
    int EPA_MAX_ITERATIONS = 32;

    bool bUSE_GJKEPA = true;
}; 