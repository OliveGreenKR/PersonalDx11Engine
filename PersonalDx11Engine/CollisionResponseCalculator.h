#pragma once
#include "Math.h"
#include "CollisionDefines.h"

using namespace DirectX;

/// <summary>
/// SIMD 최적화 충돌 반응 계산 시스템 (개선된 인터페이스)
/// Warm Starting과 통합 충격량 계산을 지원
/// </summary>
class FCollisionResponseCalculator
{
#pragma region Configuration
public:
    FCollisionResponseCalculator();

private:
    void LoadConfigFromIni();

    // 설정값들
    float RestitutionThreshold = 0.02f;        // 반발 적용 최소 속도
    float FrictionVelocityThreshold = 0.001f;  // 마찰 적용 최소 속도  
    float MaxNormalImpulse = 1000.0f;          // 법선 충격량 최대값
    float MaxFrictionImpulse = 500.0f;         // 마찰 충격량 최대값
    float WarmStartingDamping = 0.8f;          // Warm Starting 감쇠 계수

#pragma endregion

#pragma region Main Response Calculation Interface
public:
    /// <summary>
    /// 통합 충돌 반응 계산 (Warm Starting 지원)
    /// 법선 반응과 마찰을 한번에 계산하여 일관성 보장
    /// </summary>
    /// <param name="DetectResult">충돌 검출 결과</param>
    /// <param name="ParamsA">물체 A 물리 매개변수</param>
    /// <param name="ParamsB">물체 B 물리 매개변수</param>
    /// <param name="Accumulation">제약조건 누적 데이터 (in/out)</param>
    /// <param name="BiasSpeed">위치 보정 편향 속도</param>
    /// <returns>적용할 충격량들 (XMVECTOR 형태)</returns>
    FCollisionResponseResult CalculateCollisionResponse(
        const FCollisionDetectionResult& DetectResult,
        const FPhysicsParameters& ParamsA,
        const FPhysicsParameters& ParamsB,
        FCollisionAccumulation& Accumulation,
        float BiasSpeed
    );

#pragma endregion

#pragma region Individual Component Calculations (내부 사용)
private:
    /// <summary>
    /// 법선 방향 충격량 성분 계산
    /// </summary>
    XMVECTOR CalculateNormalImpulseComponent(
        const FCollisionDetectionResult& DetectResult,
        const FPhysicsParameters& ParamsA,
        const FPhysicsParameters& ParamsB,
        FCollisionAccumulation& Accumulation,
        float BiasSpeed,
        FCollisionResponseResult& OutResult
    );

    /// <summary>
    /// 접선 마찰 충격량 성분 계산
    /// </summary>
    XMVECTOR CalculateTangentFrictionComponent(
        const FCollisionDetectionResult& DetectResult,
        const FPhysicsParameters& ParamsA,
        const FPhysicsParameters& ParamsB,
        FCollisionAccumulation& Accumulation,
        float NormalLambda,
        FCollisionResponseResult& OutResult
    );

    /// <summary>
    /// 회전 마찰 충격량 성분 계산 (비틀림 저항)
    /// </summary>
    XMVECTOR CalculateTwistFrictionComponent(
        const FCollisionDetectionResult& DetectResult,
        const FPhysicsParameters& ParamsA,
        const FPhysicsParameters& ParamsB,
        FCollisionAccumulation& Accumulation,
        float NormalLambda,
        FCollisionResponseResult& OutResult
    );

#pragma endregion

#pragma region Utility Functions
private:
    /// <summary>
    /// 접촉점에서의 상대 속도 계산 (SIMD)
    /// </summary>
    XMVECTOR CalculateRelativeVelocityAtContact(
        const FPhysicsParameters& ParamsA,
        const FPhysicsParameters& ParamsB,
        XMVECTOR ContactPoint
    );

    /// <summary>
    /// 유효 질량 계산 (제약조건 방향)
    /// </summary>
    float CalculateEffectiveMass(
        const FPhysicsParameters& ParamsA,
        const FPhysicsParameters& ParamsB,
        XMVECTOR ContactPoint,
        XMVECTOR ConstraintDirection
    );

    /// <summary>
    /// 마찰 계산을 위한 접선 방향 벡터 생성
    /// </summary>
    XMVECTOR CalculateTangentVector(XMVECTOR Normal, XMVECTOR RelativeVelocity);

    /// <summary>
    /// 반발 계수 적용 여부 판단
    /// </summary>
    bool ShouldApplyRestitution(XMVECTOR RelativeVelocity, XMVECTOR Normal);

    /// <summary>
    /// 충격량 클램핑 (안전성 보장)
    /// </summary>
    XMVECTOR ClampImpulse(XMVECTOR Impulse, float MaxMagnitude);

    /// <summary>
    /// 각운동 충격량 계산 (토크 -> 각속도 변화)
    /// </summary>
    XMVECTOR CalculateAngularImpulseFromTorque(
        XMVECTOR Torque,
        XMVECTOR InvRotationalInertia
    );

#pragma endregion
};