#include "CollisionResponseCalculator.h"

FCollisionResponseResult FCollisionResponseCalculator::CalculateResponseByContraints(const FCollisionDetectionResult& DetectionResult, 
                                                                                     const FPhysicsParameters& ParameterA, const FPhysicsParameters& ParameterB,
                                                                                     FAccumulatedConstraint& Accumulation)
{
    FCollisionResponseResult ResponseData;

    if (!DetectionResult.bCollided || ParameterA.Mass < 0.0f || ParameterB.Mass < 0.0f)
        return ResponseData;

    FConstraintSolverCache ConstraintCache = PrepareConstraintCache(DetectionResult, ParameterA, ParameterB);

    //수직 충격량
    XMVECTOR vNormalImpulse = SolveNormalConstraint(DetectionResult, ConstraintCache, Accumulation);

    //마찰 충격량
    float StaticFriction = (ParameterA.FrictionStatic + ParameterB.FrictionStatic) * 0.5f;
    float KineticFriction = (ParameterA.FrictionKinetic + ParameterB.FrictionKinetic) * 0.5f;
    XMVECTOR vFrictionImpulse = SolveFrictionConstraint(ConstraintCache, StaticFriction, KineticFriction, Accumulation);

    // 최종 충격량 계산
    XMVECTOR vNetImpulse = XMVectorAdd(vNormalImpulse, vFrictionImpulse);

    // 응답 데이터 설정
    XMStoreFloat3(&ResponseData.NetImpulse, vNetImpulse);
    ResponseData.ApplicationPoint = DetectionResult.Point;

    return ResponseData;
}

XMVECTOR FCollisionResponseCalculator::CalculateRelativeVelocity(
    const Vector3& ContactPoint,
    const FPhysicsParameters& ParameterA,
    const FPhysicsParameters& ParameterB)
{                
    XMVECTOR vContactPoint = XMLoadFloat3(&ContactPoint);
    XMVECTOR vPosA = ParameterA.Position;
    XMVECTOR vPosB = ParameterB.Position;

    XMVECTOR vRadiusA = XMVectorSubtract(vContactPoint, vPosA);
    XMVECTOR vRadiusB = XMVectorSubtract(vContactPoint, vPosB);

    XMVECTOR vAngVelA = ParameterA.AngularVelocity;
    XMVECTOR vAngVelB = ParameterB.AngularVelocity;
    XMVECTOR vVelA = ParameterA.Velocity;
    XMVECTOR vVelB = ParameterB.Velocity;

    // 각속도에 의한 선속도 계산
    XMVECTOR vPointVelA = XMVector3Cross(vAngVelA, vRadiusA);
    XMVECTOR vPointVelB = XMVector3Cross(vAngVelB, vRadiusB);

    // 전체 속도 계산
    XMVECTOR vTotalVelA = XMVectorAdd(vVelA, vPointVelA);
    XMVECTOR vTotalVelB = XMVectorAdd(vVelB, vPointVelB);

    // 상대 속도 계산
    XMVECTOR vRelativeVel = XMVectorSubtract(vTotalVelB, vTotalVelA);
    return vRelativeVel;
}

FCollisionResponseCalculator::FConstraintSolverCache FCollisionResponseCalculator::PrepareConstraintCache(const FCollisionDetectionResult& DetectionResult, 
                                                                                                          const FPhysicsParameters& ParameterA, const FPhysicsParameters& ParameterB)
{
    FConstraintSolverCache cache;

    // 위치 및 속도 계산
    XMVECTOR vContactPoint = XMLoadFloat3(&DetectionResult.Point);
    XMVECTOR vPosA = ParameterA.Position;
    XMVECTOR vPosB = ParameterB.Position;

    XMVECTOR vRadiusA = XMVectorSubtract(vContactPoint, vPosA);
    XMVECTOR vRadiusB = XMVectorSubtract(vContactPoint, vPosB);

    XMVECTOR vAngVelA = ParameterA.AngularVelocity;
    XMVECTOR vAngVelB = ParameterB.AngularVelocity;
    XMVECTOR vVelA = ParameterA.Velocity;
    XMVECTOR vVelB = ParameterB.Velocity;

    // 각속도에 의한 선속도
    XMVECTOR vPointVelA = XMVector3Cross(vAngVelA, vRadiusA);
    XMVECTOR vPointVelB = XMVector3Cross(vAngVelB, vRadiusB);

    // 전체 상대 속도
    XMVECTOR vTotalVelA = XMVectorAdd(vVelA, vPointVelA);
    XMVECTOR vTotalVelB = XMVectorAdd(vVelB, vPointVelB);
    cache.vRelativeVel = XMVectorSubtract(vTotalVelB, vTotalVelA);

    // 충돌 방향
    cache.vNormal = XMLoadFloat3(&DetectionResult.Normal);
    XMVECTOR vJA_Angular = XMVector3Cross(vRadiusA, cache.vNormal);
    XMVECTOR vJB_Angular = XMVector3Cross(vRadiusB, cache.vNormal);

    // 관성 텐서 계산
    Matrix RotA = XMMatrixRotationQuaternion(ParameterA.Rotation);
    Matrix RotB = XMMatrixRotationQuaternion(ParameterB.Rotation);

    XMVECTOR vInertiaA = ParameterA.RotationalInertia;
    XMVECTOR vInertiaB = ParameterB.RotationalInertia;

    XMMATRIX InertiaTensorA_W = RotA * XMMatrixScalingFromVector(vInertiaA) * XMMatrixTranspose(RotA);
    XMMATRIX InertiaTensorB_W = RotB * XMMatrixScalingFromVector(vInertiaB) * XMMatrixTranspose(RotB);

    XMMATRIX mInvInertiaA = XMMatrixInverse(nullptr, InertiaTensorA_W);
    XMMATRIX mInvInertiaB = XMMatrixInverse(nullptr, InertiaTensorB_W);
  
    XMVECTOR JA_Angular_Inertia = XMVector3Transform(vJA_Angular, mInvInertiaA);
    XMVECTOR JB_Angular_Inertia = XMVector3Transform(vJB_Angular, mInvInertiaB);

    // 질량 관련
    float invMassA = ParameterA.Mass > KINDA_SMALL ? 1.0f / ParameterA.Mass : 0.0f;
    float invMassB = ParameterB.Mass > KINDA_SMALL ? 1.0f / ParameterB.Mass : 0.0f;

    // 유효 질량
    cache.effectiveMass = invMassA + invMassB +
        XMVectorGetX(XMVector3Dot(vJA_Angular, JA_Angular_Inertia)) +
        XMVectorGetX(XMVector3Dot(vJB_Angular, JB_Angular_Inertia));

    return cache;
}


XMVECTOR FCollisionResponseCalculator::SolveNormalConstraint(const FCollisionDetectionResult& DetectionResult, 
                                                             const FConstraintSolverCache& ConstraintCache,
                                                             FAccumulatedConstraint& Accumulation)
{
    // 라그랑주 승수 계산
    float velocityError = XMVectorGetX(XMVector3Dot(ConstraintCache.vRelativeVel, ConstraintCache.vNormal));
    float positionError = -DetectionResult.PenetrationDepth;
    float baumgarte = 0.05f; // 위치 오차 보정 계수


    //라그랑주 승수 누적
    float deltaLambda = -(velocityError + baumgarte * positionError) / ConstraintCache.effectiveMass;
    float oldLambda = Accumulation.normalLambda;
    Accumulation.normalLambda = Math::Max(oldLambda + deltaLambda, 0.0f); //음수 제한

    // 실제 적용될 델타 람다
    float appliedDelta = Accumulation.normalLambda - oldLambda;

    // 제약조건 힘 계산
    return ConstraintCache.vNormal * appliedDelta;
}

XMVECTOR FCollisionResponseCalculator::SolveFrictionConstraint(const FConstraintSolverCache& ConstraintCache,
                                                               const float StaticFriction,
                                                               const float KineticFriction,
                                                               FAccumulatedConstraint& Accumulation)
{
    // 접선 방향 계산 
    XMVECTOR vTangent = XMVector3Normalize(
        XMVectorSubtract(ConstraintCache.vRelativeVel,
                         XMVectorMultiply(ConstraintCache.vNormal,
                                          XMVector3Dot(ConstraintCache.vRelativeVel, ConstraintCache.vNormal)))
    );

    float tangentialVelocity = XMVectorGetX(XMVector3Dot(ConstraintCache.vRelativeVel, vTangent));

    // 최대 마찰력 계산
    float maxFriction = std::abs(Accumulation.normalLambda) *
        (std::abs(tangentialVelocity) < KINDA_SMALL ? StaticFriction : KineticFriction);

    // 델타 람다 계산
    float deltaLambda = -tangentialVelocity / ConstraintCache.effectiveMass;
    float oldLambda = Accumulation.frictionLambda;

    // 마찰력 제한
    Accumulation.frictionLambda = std::max(-maxFriction,
                                           std::min(maxFriction, oldLambda + deltaLambda));

    // 실제 적용될 델타
    float appliedDelta = Accumulation.frictionLambda - oldLambda;

    return XMVectorScale(vTangent, appliedDelta);
}

