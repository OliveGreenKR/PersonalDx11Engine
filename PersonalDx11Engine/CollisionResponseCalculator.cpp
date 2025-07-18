#include "CollisionResponseCalculator.h"
#include "ConfigReadManager.h"
#include "Debug.h"

using namespace DirectX;

#pragma region Configuration

FCollisionResponseCalculator::FCollisionResponseCalculator()
{
    LoadConfigFromIni();
}

void FCollisionResponseCalculator::LoadConfigFromIni()
{
    auto* configManager = UConfigReadManager::Get();
    if (!configManager)
    {
        LOG_WARNING("ConfigReadManager not available, using default response calculation values");
        return;
    }

    // 설정값 로드
    configManager->GetValue("RestitutionVelocityThreshold", RestitutionThreshold);
    configManager->GetValue("FrictionVelocityThreshold", FrictionVelocityThreshold);
    configManager->GetValue("MaxNormalImpulse", MaxNormalImpulse);
    configManager->GetValue("MaxFrictionImpulse", MaxFrictionImpulse);
    configManager->GetValue("WarmStartingDamping", WarmStartingDamping);

    // 설정값 유효성 검증
    RestitutionThreshold = std::max(0.0f, RestitutionThreshold);
    FrictionVelocityThreshold = std::max(0.0f, FrictionVelocityThreshold);
    MaxNormalImpulse = std::max(1.0f, MaxNormalImpulse);
    MaxFrictionImpulse = std::max(1.0f, MaxFrictionImpulse);
    WarmStartingDamping = std::clamp(WarmStartingDamping, 0.0f, 1.0f);

    LOG_INFO("SIMD CollisionResponseCalculator configuration loaded:");
    LOG_INFO("- Restitution Velocity Threshold: %.4f", RestitutionThreshold);
    LOG_INFO("- Friction Velocity Threshold: %.4f", FrictionVelocityThreshold);
    LOG_INFO("- Max Normal Impulse: %.2f", MaxNormalImpulse);
    LOG_INFO("- Max Friction Impulse: %.2f", MaxFrictionImpulse);
    LOG_INFO("- Warm Starting Damping: %.2f", WarmStartingDamping);
}

#pragma endregion

#pragma region Main Response Calculation Interface

FCollisionResponseResult FCollisionResponseCalculator::CalculateCollisionResponse(
    const FCollisionDetectionResult& DetectResult,
    const FPhysicsParameters& ParamsA,
    const FPhysicsParameters& ParamsB,
    FCollisionAccumulation& Accumulation,
    float BiasSpeed)
{
    FCollisionResponseResult Result;
    Result.ApplicationPoint = DetectResult.Point;

    // Warm Starting 감쇠 적용
    Accumulation.ApplyWarmStartingDamping(WarmStartingDamping);

    // 1. 법선 방향 충격량 성분 계산
    XMVECTOR NormalComponent = CalculateNormalImpulseComponent(
        DetectResult, ParamsA, ParamsB, Accumulation, BiasSpeed, Result);

    // 2. 접선 마찰 충격량 성분 계산 (법선 람다 사용)
    XMVECTOR TangentComponent = CalculateTangentFrictionComponent(
        DetectResult, ParamsA, ParamsB, Accumulation, Accumulation.NormalLambda, Result);

    // 3. 회전 마찰 충격량 성분 계산 (법선 압력 기반)
    XMVECTOR TwistComponent = CalculateTwistFrictionComponent(
        DetectResult, ParamsA, ParamsB, Accumulation, Accumulation.NormalLambda, Result);

    // 4. 모든 성분을 통합하여 최종 충격량 계산
    Result.NetImpulse = XMVectorAdd(XMVectorAdd(NormalComponent, TangentComponent), TwistComponent);

    // 5. 전체 충격량 크기 제한 (안전성 보장)
    Result.NetImpulse = ClampImpulse(Result.NetImpulse, MaxNormalImpulse + MaxFrictionImpulse);

    return Result;
}

#pragma endregion

#pragma region Individual Component Calculations

XMVECTOR FCollisionResponseCalculator::CalculateNormalImpulseComponent(
    const FCollisionDetectionResult& DetectResult,
    const FPhysicsParameters& ParamsA,
    const FPhysicsParameters& ParamsB,
    FCollisionAccumulation& Accumulation,
    float BiasSpeed,
    FCollisionResponseResult& OutResult)
{
    // 1. 접촉점에서의 상대 속도 계산 (SIMD)
    XMVECTOR RelativeVelocity = CalculateRelativeVelocityAtContact(
        ParamsA, ParamsB, DetectResult.Point);

    // 2. 법선 방향 상대 속도 (스칼라)
    XMVECTOR Normal = DetectResult.Normal;
    float RelativeNormalVelocity = XMVectorGetX(XMVector3Dot(RelativeVelocity, Normal));

    // 3. 유효 질량 계산
    float EffectiveMass = CalculateEffectiveMass(
        ParamsA, ParamsB, DetectResult.Point, Normal);

    // 4. 반발 계수 결정
    float CombinedRestitution = 0.0f;
    if (ShouldApplyRestitution(RelativeVelocity, Normal))
    {
        CombinedRestitution = std::sqrt(ParamsA.Restitution * ParamsB.Restitution);
    }

    // 5. 목표 속도 계산 (반발 + 위치 보정)
    float TargetVelocity = -(CombinedRestitution * RelativeNormalVelocity) - BiasSpeed;

    // 6. 필요한 충격량 계산
    float RequiredImpulseMagnitude = (TargetVelocity - RelativeNormalVelocity) * EffectiveMass;

    // 7. 람다 누적 (제약조건 해결 - 법선은 항상 양수)
    float OldLambda = Accumulation.NormalLambda;
    Accumulation.NormalLambda = std::max(0.0f, OldLambda + RequiredImpulseMagnitude);
    float ActualImpulseMagnitude = Accumulation.NormalLambda - OldLambda;

    // 8. 벡터 충격량 생성
    return XMVectorScale(Normal, ActualImpulseMagnitude);
}

XMVECTOR FCollisionResponseCalculator::CalculateTangentFrictionComponent(
    const FCollisionDetectionResult& DetectResult,
    const FPhysicsParameters& ParamsA,
    const FPhysicsParameters& ParamsB,
    FCollisionAccumulation& Accumulation,
    float NormalLambda,
    FCollisionResponseResult& OutResult)
{
    // 1. 법선 압력이 없으면 마찰도 없음
    if (NormalLambda <= 0.0f)
    {
        return XMVectorZero();
    }

    // 2. 상대 속도 계산
    XMVECTOR RelativeVelocity = CalculateRelativeVelocityAtContact(
        ParamsA, ParamsB, DetectResult.Point);

    // 3. 접선 방향 계산
    XMVECTOR Normal = DetectResult.Normal;
    XMVECTOR TangentVector = CalculateTangentVector(Normal, RelativeVelocity);

    // 4. 접선 방향 상대 속도
    float RelativeTangentVelocity = XMVectorGetX(XMVector3Dot(RelativeVelocity, TangentVector));

    // 5. 마찰이 적용될 만큼 충분한 속도인지 확인
    if (std::abs(RelativeTangentVelocity) < FrictionVelocityThreshold)
    {
        return XMVectorZero();
    }

    // 6. 접선 방향 유효 질량 계산
    float TangentEffectiveMass = CalculateEffectiveMass(
        ParamsA, ParamsB, DetectResult.Point, TangentVector);

    // 7. 필요한 마찰 충격량 계산
    float RequiredFrictionImpulse = -RelativeTangentVelocity * TangentEffectiveMass;

    // 8. 마찰 계수 결합
    float CombinedStaticFriction = std::sqrt(ParamsA.FrictionStatic * ParamsB.FrictionStatic);
    float CombinedKineticFriction = std::sqrt(ParamsA.FrictionKinetic * ParamsB.FrictionKinetic);

    // 9. 쿨롱 마찰 한계 계산
    float MaxStaticFriction = CombinedStaticFriction * NormalLambda;
    float MaxKineticFriction = CombinedKineticFriction * NormalLambda;

    // 10. 람다 누적 (제약조건 해결)
    float OldLambda = Accumulation.FrictionLambda;
    float NewLambda = OldLambda + RequiredFrictionImpulse;

    // 11. 정적/동적 마찰 적용
    float ClampedLambda;
    if (std::abs(NewLambda) <= MaxStaticFriction)
    {
        // 정적 마찰 (완전 정지)
        ClampedLambda = NewLambda;
    }
    else
    {
        // 동적 마찰 (슬라이딩)
        ClampedLambda = (NewLambda > 0.0f) ? MaxKineticFriction : -MaxKineticFriction;
    }

    Accumulation.FrictionLambda = ClampedLambda;
    float ActualFrictionImpulse = ClampedLambda - OldLambda;

    // 12. 벡터 충격량 생성
    return XMVectorScale(TangentVector, ActualFrictionImpulse);
}

XMVECTOR FCollisionResponseCalculator::CalculateTwistFrictionComponent(
    const FCollisionDetectionResult& DetectResult,
    const FPhysicsParameters& ParamsA,
    const FPhysicsParameters& ParamsB,
    FCollisionAccumulation& Accumulation,
    float NormalLambda,
    FCollisionResponseResult& OutResult)
{
    // 1. 법선 압력이 없으면 회전 마찰도 없음
    if (NormalLambda <= 0.0f)
    {
        return XMVectorZero();
    }

    // 2. 접촉점에서의 상대 각속도 계산
    XMVECTOR AngularVelocityA = ParamsA.AngularVelocity;
    XMVECTOR AngularVelocityB = ParamsB.AngularVelocity;
    XMVECTOR RelativeAngularVelocity = XMVectorSubtract(AngularVelocityA, AngularVelocityB);

    // 3. 법선 방향 회전 성분 (비틀림) 추출
    XMVECTOR Normal = DetectResult.Normal;
    float TwistAngularVelocity = XMVectorGetX(XMVector3Dot(RelativeAngularVelocity, Normal));

    // 4. 회전 마찰이 적용될 만큼 충분한 각속도인지 확인
    if (std::abs(TwistAngularVelocity) < FrictionVelocityThreshold)
    {
        return XMVectorZero();
    }

    // 5. 회전 방향 유효 관성 계산
    float TwistEffectiveInertia = CalculateEffectiveMass(
        ParamsA, ParamsB, DetectResult.Point, Normal);

    // 6. 필요한 회전 마찰 토크 계산
    float RequiredTwistTorque = -TwistAngularVelocity * TwistEffectiveInertia;

    // 7. 회전 마찰 계수 (일반적으로 슬라이딩 마찰보다 작음)
    float CombinedTwistFriction = std::sqrt(ParamsA.FrictionKinetic * ParamsB.FrictionKinetic) * 0.5f;

    // 8. 회전 마찰 한계 계산
    float MaxTwistFriction = CombinedTwistFriction * NormalLambda;

    // 9. 람다 누적 (제약조건 해결)
    float OldLambda = Accumulation.TwistLambda;
    float NewLambda = OldLambda + RequiredTwistTorque;

    // 10. 회전 마찰 클램핑
    float ClampedLambda = std::clamp(NewLambda, -MaxTwistFriction, MaxTwistFriction);

    Accumulation.TwistLambda = ClampedLambda;
    float ActualTwistTorque = ClampedLambda - OldLambda;

    // 11. 토크를 접촉점 충격량으로 변환
    // 비틀림 토크는 법선 방향의 회전 모멘트이므로, 접촉점에서 접선 충격량으로 근사
    XMVECTOR TangentDirection = CalculateTangentVector(Normal, RelativeAngularVelocity);
    float ContactRadius = 0.1f; // 접촉 반경 근사값 (설정값으로 추후 이동 가능)
    float TangentImpulseMagnitude = ActualTwistTorque / ContactRadius;

    return XMVectorScale(TangentDirection, TangentImpulseMagnitude);
}

#pragma endregion

#pragma region Utility Functions

XMVECTOR FCollisionResponseCalculator::CalculateRelativeVelocityAtContact(
    const FPhysicsParameters& ParamsA,
    const FPhysicsParameters& ParamsB,
    XMVECTOR ContactPoint)
{
    // 1. 물체 A의 접촉점 속도 계산
    XMVECTOR RadiusA = XMVectorSubtract(ContactPoint, ParamsA.Position);
    XMVECTOR VelocityA_Contact = XMVectorAdd(ParamsA.Velocity,
                                             XMVector3Cross(ParamsA.AngularVelocity, RadiusA));

    // 2. 물체 B의 접촉점 속도 계산
    XMVECTOR RadiusB = XMVectorSubtract(ContactPoint, ParamsB.Position);
    XMVECTOR VelocityB_Contact = XMVectorAdd(ParamsB.Velocity,
                                             XMVector3Cross(ParamsB.AngularVelocity, RadiusB));

    // 3. 상대 속도 (A - B)
    return XMVectorSubtract(VelocityA_Contact, VelocityB_Contact);
}

float FCollisionResponseCalculator::CalculateEffectiveMass(
    const FPhysicsParameters& ParamsA,
    const FPhysicsParameters& ParamsB,
    XMVECTOR ContactPoint,
    XMVECTOR ConstraintDirection)
{
    // 1. 물체 A의 기여도 계산
    XMVECTOR RadiusA = XMVectorSubtract(ContactPoint, ParamsA.Position);
    XMVECTOR CrossA = XMVector3Cross(RadiusA, ConstraintDirection);
    XMVECTOR AngularContribA = XMVector3Cross(
        XMVectorMultiply(CrossA, ParamsA.InvRotationalInertia), RadiusA);
    float EffectiveMassA = ParamsA.InvMass + XMVectorGetX(XMVector3Dot(AngularContribA, ConstraintDirection));

    // 2. 물체 B의 기여도 계산
    XMVECTOR RadiusB = XMVectorSubtract(ContactPoint, ParamsB.Position);
    XMVECTOR CrossB = XMVector3Cross(RadiusB, ConstraintDirection);
    XMVECTOR AngularContribB = XMVector3Cross(
        XMVectorMultiply(CrossB, ParamsB.InvRotationalInertia), RadiusB);
    float EffectiveMassB = ParamsB.InvMass + XMVectorGetX(XMVector3Dot(AngularContribB, ConstraintDirection));

    // 3. 전체 유효 질량 계산
    float TotalInvMass = EffectiveMassA + EffectiveMassB;
    return (TotalInvMass > KINDA_SMALL) ? (1.0f / TotalInvMass) : 0.0f;
}

XMVECTOR FCollisionResponseCalculator::CalculateTangentVector(
    XMVECTOR Normal,
    XMVECTOR RelativeVelocity)
{
    // 1. 법선 방향 성분 제거
    XMVECTOR NormalComponent = XMVectorScale(Normal,
                                             XMVectorGetX(XMVector3Dot(RelativeVelocity, Normal)));
    XMVECTOR TangentVector = XMVectorSubtract(RelativeVelocity, NormalComponent);

    // 2. 접선 벡터 정규화
    XMVECTOR TangentLength = XMVector3Length(TangentVector);
    if (XMVectorGetX(TangentLength) < KINDA_SMALL)
    {
        // 상대 속도가 법선 방향과 평행한 경우, 임의의 접선 방향 생성
        XMVECTOR ArbitraryVector = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);

        // 법선과 너무 평행한 경우 다른 축 선택
        if (std::abs(XMVectorGetX(XMVector3Dot(Normal, ArbitraryVector))) > 0.9f)
        {
            ArbitraryVector = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
        }

        TangentVector = XMVector3Cross(Normal, ArbitraryVector);
    }

    return XMVector3Normalize(TangentVector);
}

bool FCollisionResponseCalculator::ShouldApplyRestitution(
    XMVECTOR RelativeVelocity,
    XMVECTOR Normal)
{
    // 법선 방향 상대 속도 계산
    float RelativeNormalVelocity = XMVectorGetX(XMVector3Dot(RelativeVelocity, Normal));

    // 분리 속도가 임계값보다 큰 경우에만 반발 적용
    return std::abs(RelativeNormalVelocity) > RestitutionThreshold;
}

XMVECTOR FCollisionResponseCalculator::ClampImpulse(XMVECTOR Impulse, float MaxMagnitude)
{
    XMVECTOR ImpulseMagnitude = XMVector3Length(Impulse);
    float Magnitude = XMVectorGetX(ImpulseMagnitude);

    if (Magnitude > MaxMagnitude)
    {
        // 방향은 유지하고 크기만 제한
        XMVECTOR NormalizedImpulse = XMVector3Normalize(Impulse);
        return XMVectorScale(NormalizedImpulse, MaxMagnitude);
    }

    return Impulse;
}

XMVECTOR FCollisionResponseCalculator::CalculateAngularImpulseFromTorque(
    XMVECTOR Torque,
    XMVECTOR InvRotationalInertia)
{
    // 토크를 각속도 변화로 변환
    return XMVectorMultiply(Torque, InvRotationalInertia);
}

#pragma endregion