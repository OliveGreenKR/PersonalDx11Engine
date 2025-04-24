#include "CollisionResponseCalculator.h"
#include "VelocityConstraint.h"

FCollisionResponseResult FCollisionResponseCalculator::CalculateResponseByContraints(
    const FCollisionDetectionResult& DetectionResult,
    const FPhysicsParameters& ParameterA,
    const FPhysicsParameters& ParameterB,
    FAccumulatedConstraint& Accumulation)
{
    FCollisionResponseResult ResponseData;

    if (!DetectionResult.bCollided || ParameterA.Mass < 0.0f || ParameterB.Mass < 0.0f)
        return ResponseData;

    // 충돌 법선 방향 제약 조건 (반발력)
    Vector3 Normal = DetectionResult.Normal;
    FVelocityConstraint NormalConstraint(Normal, 0.0f, 0.0f);
    NormalConstraint.SetContactData(DetectionResult.Point, Normal, DetectionResult.PenetrationDepth);
    NormalConstraint.SetBias(0.05f); // Baumgarte 안정화 계수

    // 제약 조건 해결 (충격량 계산)
    float NormalLambda = Accumulation.normalLambda;
    Vector3 NormalImpulse = NormalConstraint.Solve(ParameterA, ParameterB, NormalLambda);
    Accumulation.normalLambda = NormalLambda;

    // VelocityConstraint를 사용하여 상대 속도 계산
    XMVECTOR vRelativeVel = NormalConstraint.CalculateRelativeVelocity(
        ParameterA, ParameterB, XMLoadFloat3(&DetectionResult.Point));
    XMVECTOR vNormal = XMLoadFloat3(&Normal);

    // 법선 방향 성분 제거
    XMVECTOR vNormalComponent = XMVector3Dot(vRelativeVel, vNormal);
    vNormalComponent = XMVectorMultiply(vNormal, vNormalComponent);
    XMVECTOR vTangentVel = XMVectorSubtract(vRelativeVel, vNormalComponent);

    // 접선 방향이 의미있는 크기를 가지는지 확인
    float tangentLength = XMVectorGetX(XMVector3Length(vTangentVel));
    Vector3 TangentImpulse = Vector3::Zero;

    if (tangentLength > KINDA_SMALL)
    {
        // 접선 방향 정규화
        XMVECTOR vTangent = XMVectorDivide(vTangentVel, XMVectorReplicate(tangentLength));
        Vector3 Tangent;
        XMStoreFloat3(&Tangent, vTangent);

        // 접선 방향 제약 조건 (마찰력)
        FVelocityConstraint FrictionConstraint(Tangent, 0.0f, -FLT_MAX);
        FrictionConstraint.SetContactData(DetectionResult.Point, Normal, 0.0f);

        // 마찰 제약 조건 해결
        float FrictionLambda = Accumulation.frictionLambda;
        TangentImpulse = FrictionConstraint.Solve(ParameterA, ParameterB, FrictionLambda);

        // 마찰력 크기 제한 (Coulomb 마찰 모델)
        float StaticFriction = (ParameterA.FrictionStatic + ParameterB.FrictionStatic) * 0.5f;
        float KineticFriction = (ParameterA.FrictionKinetic + ParameterB.FrictionKinetic) * 0.5f;

        // 정적/동적 마찰 결정 및 마찰 제한 적용
        ClampFriction(tangentLength, Accumulation.normalLambda, StaticFriction, KineticFriction,
                      FrictionLambda, TangentImpulse);

        Accumulation.frictionLambda = FrictionLambda;
    }

    // 최종 충격량 합산
    ResponseData.NetImpulse = NormalImpulse + TangentImpulse;
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


