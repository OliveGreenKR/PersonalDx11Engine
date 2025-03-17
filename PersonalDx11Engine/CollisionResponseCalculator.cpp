#include "CollisionResponseCalculator.h"

FCollisionResponseResult FCollisionResponseCalculator::CalculateResponseByImpulse(
    const FCollisionDetectionResult& DetectionResult,
    const FPhysicsParameters& ParameterA,
    const FPhysicsParameters& ParameterB)
{
     FCollisionResponseResult ResponseData;

    if (!DetectionResult.bCollided || ParameterA.Mass < 0.0f || ParameterB.Mass < 0.0f )
        return ResponseData;

    // 법선 벡터 로드
    XMVECTOR vNormal = XMLoadFloat3(&DetectionResult.Normal);

    // 충돌 지점에서의 상대 속도 계산
    XMVECTOR vRelativeVel = CalculateRelativeVelocity(
        DetectionResult.Point, ParameterA, ParameterB);

    // 법선 방향 속도 검사
    float normalVelocity = XMVectorGetX(XMVector3Dot(vRelativeVel, vNormal));
    if (normalVelocity > 0)
        return ResponseData;

    // 수직 충격량과 마찰 충격량 계산
    XMVECTOR vNormalImpulse = CalculateNormalImpulse(
        DetectionResult, ParameterA, ParameterB);
    XMVECTOR vFrictionImpulse = CalculateFrictionImpulse(
        vNormalImpulse, DetectionResult, ParameterA, ParameterB);

    // 최종 충격량 계산
    XMVECTOR vNetImpulse = XMVectorAdd(vNormalImpulse, vFrictionImpulse);

    // 응답 데이터 설정
    XMStoreFloat3(&ResponseData.NetImpulse, vNetImpulse);
    ResponseData.ApplicationPoint = DetectionResult.Point;

    return ResponseData;
}

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
    XMVECTOR vPosA = XMLoadFloat3(&ParameterA.Position);
    XMVECTOR vPosB = XMLoadFloat3(&ParameterB.Position);

    XMVECTOR vRadiusA = XMVectorSubtract(vContactPoint, vPosA);
    XMVECTOR vRadiusB = XMVectorSubtract(vContactPoint, vPosB);

    XMVECTOR vAngVelA = XMLoadFloat3(&ParameterA.AngularVelocity);
    XMVECTOR vAngVelB = XMLoadFloat3(&ParameterB.AngularVelocity);
    XMVECTOR vVelA = XMLoadFloat3(&ParameterA.Velocity);
    XMVECTOR vVelB = XMLoadFloat3(&ParameterB.Velocity);

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
    XMVECTOR vPosA = XMLoadFloat3(&ParameterA.Position);
    XMVECTOR vPosB = XMLoadFloat3(&ParameterB.Position);

    XMVECTOR vRadiusA = XMVectorSubtract(vContactPoint, vPosA);
    XMVECTOR vRadiusB = XMVectorSubtract(vContactPoint, vPosB);

    XMVECTOR vAngVelA = XMLoadFloat3(&ParameterA.AngularVelocity);
    XMVECTOR vAngVelB = XMLoadFloat3(&ParameterB.AngularVelocity);
    XMVECTOR vVelA = XMLoadFloat3(&ParameterA.Velocity);
    XMVECTOR vVelB = XMLoadFloat3(&ParameterB.Velocity);

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
    Matrix RotA = XMMatrixRotationQuaternion(XMLoadFloat4(&ParameterA.Rotation));
    Matrix RotB = XMMatrixRotationQuaternion(XMLoadFloat4(&ParameterB.Rotation));

    XMVECTOR vInertiaA = XMLoadFloat3(&ParameterA.RotationalInertia);
    XMVECTOR vInertiaB = XMLoadFloat3(&ParameterB.RotationalInertia);

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

XMVECTOR FCollisionResponseCalculator::CalculateNormalImpulse(
    const FCollisionDetectionResult& DetectionResult,
    const FPhysicsParameters& ParameterA,
    const FPhysicsParameters& ParameterB)
{
    // SIMD 벡터 로드
    XMVECTOR vNormal = XMLoadFloat3(&DetectionResult.Normal);
    XMVECTOR vPoint = XMLoadFloat3(&DetectionResult.Point);
    XMVECTOR vPosA = XMLoadFloat3(&ParameterA.Position);
    XMVECTOR vPosB = XMLoadFloat3(&ParameterB.Position);

    // 상대 속도 계산
    XMVECTOR vRelVel = CalculateRelativeVelocity(DetectionResult.Point, ParameterA, ParameterB);

    // 수직 방향 상대 속도
    float normalVelocity = XMVectorGetX(XMVector3Dot(vRelVel, vNormal));

    // 역질량 계산
    float invMassA = ParameterA.Mass > KINDA_SMALL ? 1.0f / ParameterA.Mass : 0.0f;
    float invMassB = ParameterB.Mass > KINDA_SMALL ? 1.0f / ParameterB.Mass : 0.0f;

    // 회전 효과 계산
    XMVECTOR vRadiusA = XMVectorSubtract(vPoint, vPosA);
    XMVECTOR vRadiusB = XMVectorSubtract(vPoint, vPosB);

    XMVECTOR vCrossA = XMVector3Cross(vRadiusA, vNormal);
    XMVECTOR vCrossB = XMVector3Cross(vRadiusB, vNormal);

    // 회전 관성 계산
    Matrix RotA = XMMatrixRotationQuaternion(XMLoadFloat4(&ParameterA.Rotation));
    Matrix RotB = XMMatrixRotationQuaternion(XMLoadFloat4(&ParameterB.Rotation));

    XMVECTOR vInertiaA = XMLoadFloat3(&ParameterA.RotationalInertia);
    XMVECTOR vInertiaB = XMLoadFloat3(&ParameterB.RotationalInertia);

    // 월드 공간으로 회전 관성 변환
    vInertiaA = XMVector3Transform(vInertiaA, RotA);
    vInertiaB = XMVector3Transform(vInertiaB, RotB);


    //각운동량 효과 계산
    XMVECTOR vAngularEffectA = XMVector3Cross(vCrossA,
                                              XMVector3Cross(vRadiusA, vNormal) / vInertiaA);
    XMVECTOR vAngularEffectB = XMVector3Cross(vCrossB,
                                              XMVector3Cross(vRadiusB, vNormal) / vInertiaB);

    //충격량 분모 계산
    XMVECTOR vAngularSum = XMVectorAdd(vAngularEffectA, vAngularEffectB);
    float impulseDenominator = invMassA + invMassB +
        XMVectorGetX(XMVector3Dot(vAngularSum, vNormal));

    //반발계수 평균으로 근사
    float Restitution = (ParameterA.Restitution + ParameterB.Restitution) * 0.5f;

    //최종 충격량 계산
    float impulseMagnitude = -(1.0f + Restitution) * normalVelocity / impulseDenominator;
    return XMVectorScale(vNormal, impulseMagnitude);
}

XMVECTOR FCollisionResponseCalculator::CalculateFrictionImpulse(
        const XMVECTOR& NormalImpulse,
        const FCollisionDetectionResult& DetectionResult,
        const FPhysicsParameters& ParameterA,
        const FPhysicsParameters& ParameterB
    )
{
    // 상대 속도 계산 
    XMVECTOR vRelVel = CalculateRelativeVelocity(DetectionResult.Point, ParameterA, ParameterB);
    XMVECTOR vNormal = XMLoadFloat3(&DetectionResult.Normal);

    // 접선 방향 속도 계산
    XMVECTOR vNormalVel = XMVectorMultiply(vNormal,
                                           XMVector3Dot(vRelVel, vNormal));
    XMVECTOR vTangentVel = XMVectorSubtract(vRelVel, vNormalVel);

    float tangentSpeed = XMVectorGetX(XMVector3Length(vTangentVel));
    if (tangentSpeed < KINDA_SMALL)
        return XMVectorSet(0, 0, 0, 0);

    XMVECTOR vTangentDir = XMVector3Normalize(vTangentVel);

    // 마찰력 계산
    float normalMagnitude = XMVectorGetX(XMVector3Length(NormalImpulse));
    float frictionMagnitude;

    float staticFriction = (ParameterA.FrictionStatic + ParameterB.FrictionStatic) * 0.5f;
    float kineticFriction = (ParameterA.FrictionKinetic + ParameterB.FrictionKinetic) * 0.5f;

    if (tangentSpeed < staticFriction * normalMagnitude)
        frictionMagnitude = tangentSpeed;
    else
        frictionMagnitude = kineticFriction * normalMagnitude;

    XMVECTOR vFrictionImpulse = XMVectorScale(vTangentDir, -frictionMagnitude);
    return vFrictionImpulse;
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

