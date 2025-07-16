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

    // 설정값 유효성 검증
    RestitutionThreshold = std::max(0.0f, RestitutionThreshold);
    FrictionVelocityThreshold = std::max(0.0f, FrictionVelocityThreshold);
    MaxNormalImpulse = std::max(1.0f, MaxNormalImpulse);
    MaxFrictionImpulse = std::max(1.0f, MaxFrictionImpulse);

    LOG_INFO("SIMD CollisionResponseCalculator configuration loaded:");
    LOG_INFO("- Restitution Velocity Threshold : %.4f", RestitutionThreshold);
    LOG_INFO("- Friction Velocity Threshold: %.4f", FrictionVelocityThreshold);
    LOG_INFO("- Max Normal Impulse: %.2f", MaxNormalImpulse);
    LOG_INFO("- Max Friction Impulse: %.2f", MaxFrictionImpulse);
}

#pragma endregion

#pragma region Main Response Calculation Interface

XMVECTOR FCollisionResponseCalculator::CalculateNormalImpulse(
    const FCollisionDetectionResult& DetectResult,
    const FPhysicsParameters& ParamsA,
    const FPhysicsParameters& ParamsB,
    float& normalLambda,
    float BiasSpeed)
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
        CombinedRestitution = (ParamsA.Restitution + ParamsB.Restitution) * 0.5f;
    }

    // 5. 목표 속도 계산 (반발 + 위치 보정)
    float TargetVelocity = -(CombinedRestitution * RelativeNormalVelocity) - BiasSpeed;

    // 6. 필요한 충격량 계산
    float RequiredImpulseMagnitude = (TargetVelocity - RelativeNormalVelocity) * EffectiveMass;

    // 7. 람다 누적 (제약조건 해결)
    float OldLambda = normalLambda;
    normalLambda = std::max(0.0f, OldLambda + RequiredImpulseMagnitude);
    float ActualImpulseMagnitude = normalLambda - OldLambda;

    // 8. 벡터 충격량 생성 및 클램핑
    XMVECTOR ImpulseVector = XMVectorScale(Normal, ActualImpulseMagnitude);
    return ClampImpulse(ImpulseVector, MaxNormalImpulse);
}

XMVECTOR FCollisionResponseCalculator::CalculateFrictionImpulse(
    const FCollisionDetectionResult& DetectResult,
    const FPhysicsParameters& ParamsA,
    const FPhysicsParameters& ParamsB,
    float normalLambda,
    float& frictionLambda)
{
    // 1. 상대 속도 계산 (SIMD)
    XMVECTOR RelativeVelocity = CalculateRelativeVelocityAtContact(
        ParamsA, ParamsB, DetectResult.Point);

    // 2. 접선 방향 계산
    XMVECTOR Normal = DetectResult.Normal;
    XMVECTOR TangentVector = CalculateTangentVector(Normal, RelativeVelocity);

    // 3. 접선 방향 상대 속도
    float RelativeTangentVelocity = XMVectorGetX(XMVector3Dot(RelativeVelocity, TangentVector));

    // 4. 마찰이 적용될 만큼 충분한 속도인지 확인
    if (std::abs(RelativeTangentVelocity) < FrictionVelocityThreshold)
    {
        return XMVectorZero();
    }

    // 5. 접선 방향 유효 질량 계산
    float TangentEffectiveMass = CalculateEffectiveMass(
        ParamsA, ParamsB, DetectResult.Point, TangentVector);

    // 6. 필요한 마찰 충격량 계산
    float RequiredFrictionImpulse = -RelativeTangentVelocity * TangentEffectiveMass;

    // 7. 마찰 계수 결합 (최대 마찰력 계산)
    float CombinedStaticFriction = (ParamsA.FrictionStatic + ParamsB.FrictionStatic) * 0.5f;
    float CombinedKineticFriction = (ParamsA.FrictionKinetic + ParamsB.FrictionKinetic) * 0.5f;

    // 8. 쿨롱 마찰 한계 계산
    float MaxStaticFriction = CombinedStaticFriction * normalLambda;
    float MaxKineticFriction = CombinedKineticFriction * normalLambda;

    // 9. 람다 누적 (제약조건 해결)
    float OldLambda = frictionLambda;
    float NewLambda = OldLambda + RequiredFrictionImpulse;

    // 10. 정적/동적 마찰 적용
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

    frictionLambda = ClampedLambda;
    float ActualFrictionImpulse = ClampedLambda - OldLambda;

    // 11. 벡터 충격량 생성 및 클램핑
    XMVECTOR FrictionImpulseVector = XMVectorScale(TangentVector, ActualFrictionImpulse);
    return ClampImpulse(FrictionImpulseVector, MaxFrictionImpulse);
}

#pragma endregion

#pragma region SIMD Utility Methods

XMVECTOR FCollisionResponseCalculator::CalculateRelativeVelocityAtContact(
    const FPhysicsParameters& ParamsA,
    const FPhysicsParameters& ParamsB,
    XMVECTOR ContactPoint)
{
    // A 물체의 접촉점 속도 (선형 + 각속도 × 반지름)
    XMVECTOR RadiusA = XMVectorSubtract(ContactPoint, ParamsA.Position);
    XMVECTOR AngularVelocityA = ParamsA.AngularVelocity;
    XMVECTOR AngularContributionA = XMVector3Cross(AngularVelocityA, RadiusA);
    XMVECTOR VelocityA = XMVectorAdd(ParamsA.Velocity, AngularContributionA);

    // B 물체의 접촉점 속도
    XMVECTOR RadiusB = XMVectorSubtract(ContactPoint, ParamsB.Position);
    XMVECTOR AngularVelocityB = ParamsB.AngularVelocity;
    XMVECTOR AngularContributionB = XMVector3Cross(AngularVelocityB, RadiusB);
    XMVECTOR VelocityB = XMVectorAdd(ParamsB.Velocity, AngularContributionB);

    // 상대 속도 = A속도 - B속도
    return XMVectorSubtract(VelocityA, VelocityB);
}

float FCollisionResponseCalculator::CalculateEffectiveMass(
    const FPhysicsParameters& ParamsA,
    const FPhysicsParameters& ParamsB,
    XMVECTOR ContactPoint,
    XMVECTOR Normal)
{
    // A 물체 기여도 계산
    float InvMassA = ParamsA.InvMass;
    XMVECTOR RadiusA = XMVectorSubtract(ContactPoint, ParamsA.Position);
    XMVECTOR CrossA = XMVector3Cross(RadiusA, Normal);

    // 회전 관성 텐서를 대각 행렬로 가정 (SIMD 최적화)
    XMVECTOR InvInertiaA = ParamsA.InvRotationalInertia;
    XMVECTOR RotationalContribA = XMVectorMultiply(CrossA, InvInertiaA);
    XMVECTOR RotationalTermA_Vec = XMVector3Cross(RotationalContribA, RadiusA);
    float RotationalTermA = XMVectorGetX(XMVector3Dot(RotationalTermA_Vec, Normal));

    // B 물체 기여도 계산
    float InvMassB = ParamsB.InvMass;
    XMVECTOR RadiusB = XMVectorSubtract(ContactPoint, ParamsB.Position);
    XMVECTOR CrossB = XMVector3Cross(RadiusB, Normal);

    XMVECTOR InvInertiaB = ParamsB.InvRotationalInertia;
    XMVECTOR RotationalContribB = XMVectorMultiply(CrossB, InvInertiaB);
    XMVECTOR RotationalTermB_Vec = XMVector3Cross(RotationalContribB, RadiusB);
    float RotationalTermB = XMVectorGetX(XMVector3Dot(RotationalTermB_Vec, Normal));

    // 유효 역질량 = 선형 역질량 + 회전 기여도
    float EffectiveInvMass = InvMassA + InvMassB + RotationalTermA + RotationalTermB;

    // 안전성 검사
    if (EffectiveInvMass < KINDA_SMALL)
    {
        LOG_WARNING("CalculateEffectiveMass: Very small effective mass detected (%.8f)", EffectiveInvMass);
        return 1.0f; // 기본값 반환
    }

    return 1.0f / EffectiveInvMass;
}

XMVECTOR FCollisionResponseCalculator::CalculateTangentVector(
    XMVECTOR Normal,
    XMVECTOR RelativeVelocity)
{
    // 1. 법선 방향 성분 제거
    XMVECTOR NormalComponent = XMVectorScale(Normal, XMVectorGetX(XMVector3Dot(RelativeVelocity, Normal)));
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

#pragma endregion

#pragma region Advanced Response Features

#pragma region Advanced Response Features

XMVECTOR FCollisionResponseCalculator::CalculateRollingFriction(
    const FPhysicsParameters& ParamsA,
    const FPhysicsParameters& ParamsB,
    XMVECTOR ContactPoint,
    XMVECTOR Normal,
    float normalLambda)
{
    // 1. 법선 충격량이 없으면 회전 마찰도 없음
    if (normalLambda <= 0.0f)
    {
        return XMVectorZero();
    }

    // 2. 접촉점에서의 상대 각속도 계산
    XMVECTOR AngularVelocityA = ParamsA.AngularVelocity;
    XMVECTOR AngularVelocityB = ParamsB.AngularVelocity;
    XMVECTOR RelativeAngularVelocity = XMVectorSubtract(AngularVelocityA, AngularVelocityB);

    // 3. 접촉 평면에서의 회전 마찰 계산
    // 법선에 수직인 회전 성분만 고려
    XMVECTOR TangentialAngularVelocity = XMVectorSubtract(
        RelativeAngularVelocity,
        XMVectorScale(Normal, XMVectorGetX(XMVector3Dot(RelativeAngularVelocity, Normal)))
    );

    // 4. 회전 마찰이 적용될 만큼 충분한 각속도인지 확인
    XMVECTOR AngularSpeed = XMVector3Length(TangentialAngularVelocity);
    if (XMVectorGetX(AngularSpeed) < FrictionVelocityThreshold)
    {
        return XMVectorZero();
    }

    // 5. 회전 마찰 계수 (일반적으로 슬라이딩 마찰보다 작음)
    float CombinedRollingFriction = (ParamsA.FrictionKinetic + ParamsB.FrictionKinetic) * 0.25f; // 롤링 마찰은 더 작음

    // 6. 접촉점에서의 반지름 계산
    XMVECTOR RadiusA = XMVectorSubtract(ContactPoint, ParamsA.Position);
    XMVECTOR RadiusB = XMVectorSubtract(ContactPoint, ParamsB.Position);

    // 평균 반지름 사용 (회전 마찰 계산용)
    float AvgRadius = (XMVectorGetX(XMVector3Length(RadiusA)) + XMVectorGetX(XMVector3Length(RadiusB))) * 0.5f;

    // 7. 회전 마찰 토크 계산
    // T = μ_roll * normalLambda * r * ω_direction
    // normalLambda가 접촉 압력을 나타냄
    float RollingTorqueMagnitude = CombinedRollingFriction * normalLambda * AvgRadius;

    // 8. 토크 방향 (각속도 반대 방향)
    XMVECTOR TorqueDirection = XMVector3Normalize(XMVectorNegate(TangentialAngularVelocity));

    // 9. 최종 회전 마찰 토크를 각충격량으로 변환
    // 회전 마찰은 접촉점에서 토크로 작용
    return XMVectorScale(TorqueDirection, RollingTorqueMagnitude);
}

#pragma endregion

#pragma endregion