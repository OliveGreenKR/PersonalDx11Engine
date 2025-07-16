#pragma once
#include "Math.h"
#include "Transform.h"
#include "CollisionDefines.h"

using namespace DirectX;

struct FMAABB;

/// <summary>
/// SIMD 최적화 충돌 검출 시스템
/// 내부 시스템용 XMVECTOR 기반 고성능 구현
/// Position + Rotation 분리 저장 방식 사용
/// </summary>
class FCollisionDetector
{
#pragma region Configuration
public:
    FCollisionDetector();

private:
    void LoadConfigFromIni();

    // 설정값
    float CCDTimeStep = 0.02f;         // CCD 시간 스텝
    int MaxCCDIterations = 10;         // CCD 최대 반복 횟수
    float DistanceThreshold = 0.001f;  // 접촉 간주 거리 임계값

#pragma endregion

#pragma region Main Detection Interface
public:
    /// <summary>
    /// 이산 충돌 검출 (현재 프레임 기준)
    /// </summary>
    /// <param name="shapeDataA">형상 A 데이터 (SIMD 최적화)</param>
    /// <param name="shapeDataB">형상 B 데이터 (SIMD 최적화)</param>
    /// <returns>충돌 검출 결과</returns>
    FCollisionDetectionResult DetectCollisionDiscrete(
        const FCollisionShapeData& shapeDataA,
        const FCollisionShapeData& shapeDataB);

    /// <summary>
    /// 연속 충돌 검출 (CCD)
    /// </summary>
    /// <param name="shapeDataA">형상 A 데이터 (Prev/Current 위치 사용)</param>
    /// <param name="shapeDataB">형상 B 데이터 (Prev/Current 위치 사용)</param>
    /// <param name="deltaTime">프레임 시간</param>
    /// <returns>충돌 검출 결과</returns>
    FCollisionDetectionResult DetectCollisionCCD(
        const FCollisionShapeData& shapeDataA,
        const FCollisionShapeData& shapeDataB,
        float deltaTime);

#pragma endregion

#pragma region SIMD Shape-Based Detection Methods
public:
    /// <summary>
    /// Sphere-Sphere 충돌 검출 (SIMD 최적화)
    /// </summary>
    FCollisionDetectionResult SphereSphere(
        float radiusA, XMVECTOR positionA,
        float radiusB, XMVECTOR positionB);

    /// <summary>
    /// Box-Box 충돌 검출 (SAT 알고리즘, SIMD 최적화)
    /// </summary>
    FCollisionDetectionResult BoxBoxSAT(
        XMVECTOR extentA, XMVECTOR positionA, XMVECTOR rotationA,
        XMVECTOR extentB, XMVECTOR positionB, XMVECTOR rotationB);

    /// <summary>
    /// Box-Sphere 충돌 검출 (SIMD 최적화)
    /// </summary>
    FCollisionDetectionResult BoxSphereSimple(
        XMVECTOR boxExtent, XMVECTOR boxPosition, XMVECTOR boxRotation,
        float sphereRadius, XMVECTOR spherePosition);

#pragma endregion

#pragma region SIMD Utility Methods
private:

    /// <summary>
    /// Swept AABB 계산 (SIMD)
    /// </summary>
    FMAABB CalculateSweptAABB(const FCollisionShapeData& shapeData) const;

    /// <summary>
    /// 월드 AABB 계산 (SIMD, 지정된 위치/회전 사용)
    /// </summary>
    FMAABB CalculateWorldAABB(const FCollisionShapeData& shapeData,
                             XMVECTOR position, XMVECTOR rotation) const;

    /// <summary>
    /// 현재 위치 기준 월드 AABB 계산 (SIMD)
    /// </summary>
    FMAABB CalculateCurrentWorldAABB(const FCollisionShapeData& shapeData) const;

    /// <summary>
    /// 회전 매트릭스 생성 (SIMD)
    /// </summary>
    XMMATRIX CreateRotationMatrix(XMVECTOR rotation) const;

    /// <summary>
    /// Transform 매트릭스 생성 (SIMD)
    /// </summary>
    XMMATRIX CreateTransformMatrix(XMVECTOR position, XMVECTOR rotation) const;

#pragma endregion
};