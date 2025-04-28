#include "CollisionResponseCalculator.h"
#include "VelocityConstraint.h"
#include "PhysicsStateInterface.h"

// CollisionResponseCalculator.cpp 수정
FCollisionResponseResult FCollisionResponseCalculator::CalculateResponseByContraints(
    const FCollisionDetectionResult& DetectionResult,
    const FPhysicsParameters& ParameterA,
    const FPhysicsParameters& ParameterB,
    FAccumulatedConstraint& Accumulation)
{
    FCollisionResponseResult ResponseData;

    if (!DetectionResult.bCollided || ParameterA.Mass < 0.0f || ParameterB.Mass < 0.0f)
        return ResponseData;

    // 기존 누적 람다값으로 시작 (Warm Starting)
    float NormalLambda = Accumulation.normalLambda;
    float FrictionLambda = Accumulation.frictionLambda;

    // 1. 충돌 법선 방향 제약 조건 설정
    Vector3 Normal = DetectionResult.Normal;
    FVelocityConstraint NormalConstraint(Normal, 0.0f, 0.0f);
    NormalConstraint.SetContactData(DetectionResult.Point, Normal, DetectionResult.PenetrationDepth);

    // 반발 계수 적용 (두 물체의 평균)
    float RestitutionCoef = std::min(0.9999f,(ParameterA.Restitution + ParameterB.Restitution) * 0.5f);
    float BiasFactor = 0.05f;  // 위치 보정 계수

    // 상대 속도가 임계값 이하일 때는 반발 계수를 0으로 설정 (마찰과 함께 정지 상태 유지)
    XMVECTOR vRelativeVel = NormalConstraint.CalculateRelativeVelocity(
        ParameterA, ParameterB, XMLoadFloat3(&DetectionResult.Point));
    XMVECTOR vNormal = XMLoadFloat3(&Normal);
    float NormalVelocity = XMVectorGetX(XMVector3Dot(vRelativeVel, vNormal));

    // 속도가 임계값 이하면 반발 무시 (정지 상태 유지)
    const float VelocityThreshold = KINDA_SMALL;
    if (std::abs(NormalVelocity) < VelocityThreshold)
        RestitutionCoef = 0.0f;

    // 반발 속도 목표 설정 (충돌 속도에 반발 계수 적용)
    float DesiredVelocity = NormalVelocity < 0.0f ? -NormalVelocity * RestitutionCoef : 0.0f;
    NormalConstraint.SetDesiredSpeed(DesiredVelocity);
    NormalConstraint.SetBias(BiasFactor);  // 위치 오차 보정 계수

    // 제약 조건 해결 (충격량 계산)
    Vector3 NormalImpulse = NormalConstraint.Solve(ParameterA, ParameterB, NormalLambda);
    Accumulation.normalLambda = NormalLambda;

    // 2. 접선 방향 처리 (마찰)
    Vector3 TangentImpulse = Vector3::Zero;

    // 법선 방향 성분 제거하여 접선 방향 계산
    XMVECTOR vNormalComponent = XMVectorScale(vNormal, NormalVelocity);
    XMVECTOR vTangentVel = XMVectorSubtract(vRelativeVel, vNormalComponent);

    float tangentLength = XMVectorGetX(XMVector3Length(vTangentVel));
    if (tangentLength > KINDA_SMALL)
    {
        // 접선 방향 정규화
        XMVECTOR vTangent = XMVectorDivide(vTangentVel, XMVectorReplicate(tangentLength));
        Vector3 Tangent;
        XMStoreFloat3(&Tangent, vTangent);

        // 접선 방향 제약 조건 (마찰력)
        FVelocityConstraint FrictionConstraint(Tangent, 0.0f);
        FrictionConstraint.SetContactData(DetectionResult.Point, Normal, 0.0f);

        // Warm Starting: 이전 프레임의 마찰 람다 재사용
        TangentImpulse = FrictionConstraint.Solve(ParameterA, ParameterB, FrictionLambda);

        // 마찰력 크기 제한 (Coulomb 마찰 모델)
        float StaticFriction = (ParameterA.FrictionStatic + ParameterB.FrictionStatic) * 0.5f;
        float KineticFriction = (ParameterA.FrictionKinetic + ParameterB.FrictionKinetic) * 0.5f;

        ClampFriction(tangentLength, NormalLambda, StaticFriction, KineticFriction,
                      FrictionLambda, TangentImpulse);

        Accumulation.frictionLambda = FrictionLambda;
    }
    
    // 최종 충격량 합산
    ResponseData.NetImpulse = (NormalImpulse + TangentImpulse);
    ResponseData.ApplicationPoint = DetectionResult.Point;

    return ResponseData;
}

void FCollisionResponseCalculator::ClampFriction(
     float TangentVelocityLength,
    const float NormalLambda,
    const float StaticFriction,
    const float KineticFriction,
    float& OutFrictionLambda,
    Vector3& OutTangentImpulse)
{
    // 현재 속도에 따라 적절한 마찰 계수 선택 (정적 또는 동적)
    float FrictionCoefficient = (TangentVelocityLength < KINDA_SMALL) ?
        StaticFriction : KineticFriction;

    // Coulomb 마찰 모델: 마찰력 <= 수직항력 * 마찰계수
    float MaxFriction = std::abs(NormalLambda) * FrictionCoefficient;

    // 현재 마찰력의 크기 확인
    float CurrentFrictionMagnitude = std::abs(OutFrictionLambda);

    // 마찰력이 최대값을 초과하는 경우 제한 적용
    if (CurrentFrictionMagnitude > MaxFriction)
    {
        // 최대값으로 스케일링
        float Scale = MaxFriction / CurrentFrictionMagnitude;

        // 충격량과 람다값 모두 스케일링
        OutTangentImpulse = OutTangentImpulse * Scale;

        // 방향은 유지하면서 크기만 제한
        float Sign = (OutFrictionLambda >= 0) ? 1.0f : -1.0f;
        OutFrictionLambda = MaxFriction * Sign;
    }
}


