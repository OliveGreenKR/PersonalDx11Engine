#pragma once
#include "Math.h"
#include "Transform.h"
#include "CollisionDefines.h"


// 충돌 검사 알고리즘 모음
class  FCollisionDetector
{

    struct FSimplex
    {
        Vector3 Points[4];          // 최대 4개의 점(테트라헤드론)
        Vector3 MinkowskiPoints[4]; // 민코프스키 차 공간의 실제 점
        int Size;                   // 현재 심플렉스의 점 개수
    };

    // EPA 내부 구조체 - 면을 표현
    struct FFace
    {
        int Indices[3];       // 심플렉스 점들의 인덱스
        Vector3 Normal;       // 면의 법선 벡터
        float Distance;       // 원점에서 면까지의 거리

        bool operator<(const FFace& Other) const
        {
            return Distance < Other.Distance;
        }
    };

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

private:
    // 서포트 함수 - 특정 방향으로 가장 멀리 있는 점 반환
    Vector3 Support(
        const FCollisionShapeData& ShapeA, const FTransform& TransformA,
        const FCollisionShapeData& ShapeB, const FTransform& TransformB,
        const Vector3& Direction);

    // 형상별 서포트 함수
    Vector3 SupportForShape(
        const FCollisionShapeData& Shape,
        const FTransform& Transform,
        const Vector3& Direction);

    // GJK 알고리즘 - 두 볼록 형상의 충돌 여부 검사
    bool GJK(
        const FCollisionShapeData& ShapeA, const FTransform& TransformA,
        const FCollisionShapeData& ShapeB, const FTransform& TransformB,
        FSimplex& OutSimplex);

    // EPA 알고리즘 - 침투 깊이와 충돌 법선 계산
    FCollisionDetectionResult EPA(
        const FCollisionShapeData& ShapeA, const FTransform& TransformA,
        const FCollisionShapeData& ShapeB, const FTransform& TransformB,
        FSimplex& Simplex);

    // GJK 내부 함수 - 다음 방향 찾기
    bool ProcessSimplex(FSimplex& Simplex, Vector3& Direction);

    // EPA 내부 헬퍼 함수들
    std::vector<FFace> BuildInitialPolyhedron(const FSimplex& Simplex);
    int FindClosestFace(const std::vector<FFace>& Faces);
    void ExpandPolyhedron(
        std::vector<FFace>& Faces,
        const Vector3& NewPoint,
        const FCollisionShapeData& ShapeA, const FTransform& TransformA,
        const FCollisionShapeData& ShapeB, const FTransform& TransformB);

    // GJK+EPA 통합 충돌 감지 함수
    FCollisionDetectionResult DetectCollisionGJKEPA(
        const FCollisionShapeData& ShapeA, const FTransform& TransformA,
        const FCollisionShapeData& ShapeB, const FTransform& TransformB);
public:
    float TimeStep = 0.02f;

    float GJK_EPSILON = 0.0001f;
    float EPA_EPSILON = 0.0001f;
    int EPA_MAX_ITERATIONS = 32;

    bool bUseGJKEPA = true;
}; 