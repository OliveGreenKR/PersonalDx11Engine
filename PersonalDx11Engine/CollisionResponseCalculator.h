#pragma once
#include "Math.h"
#include "CollisionDefines.h"

using namespace DirectX;

/// <summary>
/// SIMD 최적화 충돌 반응 계산 시스템
/// 내부 시스템용 XMVECTOR 기반 고성능 구현
/// PhysicsID 의존성 없는 순수 계산 함수들
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

#pragma endregion

#pragma region Main Response Calculation Interface
public:
    /// <summary>
    /// 법선 방향 충격량 계산 (SIMD 최적화)
    /// </summary>
    /// <param name="DetectResult">충돌 검출 결과 (SIMD 데이터 포함)</param>
    /// <param name="ParamsA">물체 A 물리 매개변수</param>
    /// <param name="ParamsB">물체 B 물리 매개변수</param>
    /// <param name="normalLambda">제약조건 람다값 (in/out)</param>
    /// <param name="BiasSpeed">위치 보정 편향 속도</param>
    /// <returns>법선 충격량 (XMVECTOR)</returns>
    XMVECTOR CalculateNormalImpulse(
        const FCollisionDetectionResult& DetectResult,
        const FPhysicsParameters& ParamsA,
        const FPhysicsParameters& ParamsB,
        float& normalLambda,
        float BiasSpeed
    );

    /// <summary>
    /// 마찰 방향 충격량 계산 (SIMD 최적화)
    /// </summary>
    /// <param name="DetectResult">충돌 검출 결과</param>
    /// <param name="ParamsA">물체 A 물리 매개변수</param>
    /// <param name="ParamsB">물체 B 물리 매개변수</param>
    /// <param name="normalLambda">법선 람다값 (마찰 한계 계산용)</param>
    /// <param name="frictionLambda">마찰 람다값 (in/out)</param>
    /// <returns>마찰 충격량 (XMVECTOR)</returns>
    XMVECTOR CalculateFrictionImpulse(
        const FCollisionDetectionResult& DetectResult,
        const FPhysicsParameters& ParamsA,
        const FPhysicsParameters& ParamsB,
        float normalLambda,
        float& frictionLambda
    );

#pragma endregion

#pragma region SIMD Utility Methods
private:
    /// <summary>
    /// 상대 속도 계산 (SIMD 최적화)
    /// 접촉점에서의 두 물체간 상대 속도 벡터 계산
    /// </summary>
    /// <param name="ParamsA">물체 A 매개변수</param>
    /// <param name="ParamsB">물체 B 매개변수</param>
    /// <param name="ContactPoint">접촉점 (XMVECTOR)</param>
    /// <returns>상대 속도 벡터</returns>
    XMVECTOR CalculateRelativeVelocityAtContact(
        const FPhysicsParameters& ParamsA,
        const FPhysicsParameters& ParamsB,
        XMVECTOR ContactPoint
    );

    /// <summary>
    /// 유효 질량 계산 (SIMD 최적화)
    /// 접촉점과 법선 방향에서의 유효 질량 계산
    /// </summary>
    /// <param name="ParamsA">물체 A 매개변수</param>
    /// <param name="ParamsB">물체 B 매개변수</param>
    /// <param name="ContactPoint">접촉점</param>
    /// <param name="Normal">접촉 법선</param>
    /// <returns>유효 질량</returns>
    float CalculateEffectiveMass(
        const FPhysicsParameters& ParamsA,
        const FPhysicsParameters& ParamsB,
        XMVECTOR ContactPoint,
        XMVECTOR Normal
    );

    /// <summary>
    /// 접선 벡터 계산 (SIMD 최적화)
    /// 마찰 계산을 위한 접선 방향 벡터 생성
    /// </summary>
    /// <param name="Normal">접촉 법선</param>
    /// <param name="RelativeVelocity">상대 속도</param>
    /// <returns>정규화된 접선 벡터</returns>
    XMVECTOR CalculateTangentVector(
        XMVECTOR Normal,
        XMVECTOR RelativeVelocity
    );

    /// <summary>
    /// 반발 계수 적용 여부 판단
    /// </summary>
    /// <param name="RelativeVelocity">상대 속도</param>
    /// <param name="Normal">접촉 법선</param>
    /// <returns>반발 적용 여부</returns>
    bool ShouldApplyRestitution(
        XMVECTOR RelativeVelocity,
        XMVECTOR Normal
    );

    /// <summary>
    /// 충격량 클램핑 (안전성 보장)
    /// </summary>
    /// <param name="Impulse">원본 충격량</param>
    /// <param name="MaxMagnitude">최대 크기</param>
    /// <returns>클램핑된 충격량</returns>
    XMVECTOR ClampImpulse(XMVECTOR Impulse, float MaxMagnitude);

#pragma endregion

#pragma region Advanced Response Features  
public:
    /// <summary>
    /// 회전 마찰 계산 (고급 기능)
    /// 접촉점에서의 회전 마찰 효과 계산
    /// </summary>
    /// <param name="ParamsA">물체 A 매개변수</param>
    /// <param name="ParamsB">물체 B 매개변수</param>
    /// <param name="ContactPoint">접촉점</param>
    /// <param name="Normal">접촉 법선</param>
    /// <param name="normalLambda">법선 충격량 (접촉 압력 기준)</param>
    /// <returns>회전 마찰 토크</returns>
    XMVECTOR CalculateRollingFriction(
        const FPhysicsParameters& ParamsA,
        const FPhysicsParameters& ParamsB,
        XMVECTOR ContactPoint,
        XMVECTOR Normal,
        float normalLambda
    );

#pragma endregion
};