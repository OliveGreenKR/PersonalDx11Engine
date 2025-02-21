#include "CollisionResponseCalculator.h"

FCollisionResponseResult FCollisionResponseCalculator::CalculateResponseByImpulse(
    const FCollisionDetectionResult& DetectionResult,
    const FCollisionResponseParameters& ParameterA,
    const FCollisionResponseParameters& ParameterB)
{
     FCollisionResponseResult ResponseData;

    if (!DetectionResult.bCollided || ParameterA.Mass < 0.0f || ParameterB.Mass < 0.0f )
        return ResponseData;

    // ���� ���� �ε�
    XMVECTOR vNormal = XMLoadFloat3(&DetectionResult.Normal);

    // �浹 ���������� ��� �ӵ� ���
    XMVECTOR vRelativeVel = CalculateRelativeVelocity(
        DetectionResult.Point, ParameterA, ParameterB);

    // ���� ���� �ӵ� �˻�
    float normalVelocity = XMVectorGetX(XMVector3Dot(vRelativeVel, vNormal));
    if (normalVelocity > 0)
        return ResponseData;

    // ���� ��ݷ��� ���� ��ݷ� ���
    XMVECTOR vNormalImpulse = CalculateNormalImpulse(
        DetectionResult, ParameterA, ParameterB);
    XMVECTOR vFrictionImpulse = CalculateFrictionImpulse(
        vNormalImpulse, DetectionResult, ParameterA, ParameterB);

    // ���� ��ݷ� ���
    XMVECTOR vNetImpulse = XMVectorAdd(vNormalImpulse, vFrictionImpulse);

    // ���� ������ ����
    XMStoreFloat3(&ResponseData.NetImpulse, vNetImpulse);
    ResponseData.ApplicationPoint = DetectionResult.Point;

    return ResponseData;
}

FCollisionResponseResult FCollisionResponseCalculator::CalculateResponseByContraints(const FCollisionDetectionResult& DetectionResult, 
                                                                                     const FCollisionResponseParameters& ParameterA, const FCollisionResponseParameters& ParameterB,
                                                                                     FAccumulatedConstraint& Accumulation)
{
    FCollisionResponseResult ResponseData;

    if (!DetectionResult.bCollided || ParameterA.Mass < 0.0f || ParameterB.Mass < 0.0f)
        return ResponseData;

    FConstraintSolverCache ConstraintCache = PrepareConstraintCache(DetectionResult, ParameterA, ParameterB);

    //���� ��ݷ�
    float NormalLamda;
    XMVECTOR vNormalImpulse = SolveNormalConstraint(DetectionResult, ConstraintCache, Accumulation);

    //���� ��ݷ�
    float StaticFriction = (ParameterA.FrictionStatic + ParameterB.FrictionStatic) * 0.5f;
    float KineticFriction = (ParameterA.FrictionKinetic + ParameterB.FrictionKinetic) * 0.5f;
    XMVECTOR vFrictionImpulse = SolveFrictionConstraint(ConstraintCache, StaticFriction, KineticFriction, Accumulation);

    // ���� ��ݷ� ���
    XMVECTOR vNetImpulse = XMVectorAdd(vNormalImpulse, vFrictionImpulse);

    // ���� ������ ����
    XMStoreFloat3(&ResponseData.NetImpulse, vNetImpulse);
    ResponseData.ApplicationPoint = DetectionResult.Point;

    return ResponseData;
}

XMVECTOR FCollisionResponseCalculator::CalculateRelativeVelocity(
    const Vector3& ContactPoint,
    const FCollisionResponseParameters& ParameterA,
    const FCollisionResponseParameters& ParameterB)
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

    // ���ӵ��� ���� ���ӵ� ���
    XMVECTOR vPointVelA = XMVector3Cross(vAngVelA, vRadiusA);
    XMVECTOR vPointVelB = XMVector3Cross(vAngVelB, vRadiusB);

    // ��ü �ӵ� ���
    XMVECTOR vTotalVelA = XMVectorAdd(vVelA, vPointVelA);
    XMVECTOR vTotalVelB = XMVectorAdd(vVelB, vPointVelB);

    // ��� �ӵ� ���
    XMVECTOR vRelativeVel = XMVectorSubtract(vTotalVelB, vTotalVelA);
    return vRelativeVel;
}

FCollisionResponseCalculator::FConstraintSolverCache FCollisionResponseCalculator::PrepareConstraintCache(const FCollisionDetectionResult& DetectionResult, 
                                                                                                          const FCollisionResponseParameters& ParameterA, const FCollisionResponseParameters& ParameterB)
{
    FConstraintSolverCache cache;

    // ��ġ �� �ӵ� ���
    XMVECTOR vContactPoint = XMLoadFloat3(&DetectionResult.Point);
    XMVECTOR vPosA = XMLoadFloat3(&ParameterA.Position);
    XMVECTOR vPosB = XMLoadFloat3(&ParameterB.Position);

    XMVECTOR vRadiusA = XMVectorSubtract(vContactPoint, vPosA);
    XMVECTOR vRadiusB = XMVectorSubtract(vContactPoint, vPosB);

    XMVECTOR vAngVelA = XMLoadFloat3(&ParameterA.AngularVelocity);
    XMVECTOR vAngVelB = XMLoadFloat3(&ParameterB.AngularVelocity);
    XMVECTOR vVelA = XMLoadFloat3(&ParameterA.Velocity);
    XMVECTOR vVelB = XMLoadFloat3(&ParameterB.Velocity);

    // ���ӵ��� ���� ���ӵ�
    XMVECTOR vPointVelA = XMVector3Cross(vAngVelA, vRadiusA);
    XMVECTOR vPointVelB = XMVector3Cross(vAngVelB, vRadiusB);

    // ��ü ��� �ӵ�
    XMVECTOR vTotalVelA = XMVectorAdd(vVelA, vPointVelA);
    XMVECTOR vTotalVelB = XMVectorAdd(vVelB, vPointVelB);
    cache.vRelativeVel = XMVectorSubtract(vTotalVelB, vTotalVelA);

    // �浹 ����
    cache.vNormal = XMLoadFloat3(&DetectionResult.Normal);
    XMVECTOR vJA_Angular = XMVector3Cross(vRadiusA, cache.vNormal);
    XMVECTOR vJB_Angular = XMVector3Cross(vRadiusB, cache.vNormal);

    // ���� �ټ� ���
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

    // ���� ����
    float invMassA = ParameterA.Mass > KINDA_SMALL ? 1.0f / ParameterA.Mass : 0.0f;
    float invMassB = ParameterB.Mass > KINDA_SMALL ? 1.0f / ParameterB.Mass : 0.0f;

    // ��ȿ ����
    cache.effectiveMass = invMassA + invMassB +
        XMVectorGetX(XMVector3Dot(vJA_Angular, JA_Angular_Inertia)) +
        XMVectorGetX(XMVector3Dot(vJB_Angular, JB_Angular_Inertia));

    return cache;
}

XMVECTOR FCollisionResponseCalculator::CalculateNormalImpulse(
    const FCollisionDetectionResult& DetectionResult,
    const FCollisionResponseParameters& ParameterA,
    const FCollisionResponseParameters& ParameterB)
{
    // SIMD ���� �ε�
    XMVECTOR vNormal = XMLoadFloat3(&DetectionResult.Normal);
    XMVECTOR vPoint = XMLoadFloat3(&DetectionResult.Point);
    XMVECTOR vPosA = XMLoadFloat3(&ParameterA.Position);
    XMVECTOR vPosB = XMLoadFloat3(&ParameterB.Position);

    // ��� �ӵ� ���
    XMVECTOR vRelVel = CalculateRelativeVelocity(DetectionResult.Point, ParameterA, ParameterB);

    // ���� ���� ��� �ӵ�
    float normalVelocity = XMVectorGetX(XMVector3Dot(vRelVel, vNormal));

    // ������ ���
    float invMassA = ParameterA.Mass > KINDA_SMALL ? 1.0f / ParameterA.Mass : 0.0f;
    float invMassB = ParameterB.Mass > KINDA_SMALL ? 1.0f / ParameterB.Mass : 0.0f;

    // ȸ�� ȿ�� ���
    XMVECTOR vRadiusA = XMVectorSubtract(vPoint, vPosA);
    XMVECTOR vRadiusB = XMVectorSubtract(vPoint, vPosB);

    XMVECTOR vCrossA = XMVector3Cross(vRadiusA, vNormal);
    XMVECTOR vCrossB = XMVector3Cross(vRadiusB, vNormal);

    // ȸ�� ���� ���
    Matrix RotA = XMMatrixRotationQuaternion(XMLoadFloat4(&ParameterA.Rotation));
    Matrix RotB = XMMatrixRotationQuaternion(XMLoadFloat4(&ParameterB.Rotation));

    XMVECTOR vInertiaA = XMLoadFloat3(&ParameterA.RotationalInertia);
    XMVECTOR vInertiaB = XMLoadFloat3(&ParameterB.RotationalInertia);

    // ���� �������� ȸ�� ���� ��ȯ
    vInertiaA = XMVector3Transform(vInertiaA, RotA);
    vInertiaB = XMVector3Transform(vInertiaB, RotB);


    //����� ȿ�� ���
    XMVECTOR vAngularEffectA = XMVector3Cross(vCrossA,
                                              XMVector3Cross(vRadiusA, vNormal) / vInertiaA);
    XMVECTOR vAngularEffectB = XMVector3Cross(vCrossB,
                                              XMVector3Cross(vRadiusB, vNormal) / vInertiaB);

    //��ݷ� �и� ���
    XMVECTOR vAngularSum = XMVectorAdd(vAngularEffectA, vAngularEffectB);
    float impulseDenominator = invMassA + invMassB +
        XMVectorGetX(XMVector3Dot(vAngularSum, vNormal));

    //�ݹ߰�� ������� �ٻ�
    float Restitution = (ParameterA.Restitution + ParameterB.Restitution) * 0.5f;

    //���� ��ݷ� ���
    float impulseMagnitude = -(1.0f + Restitution) * normalVelocity / impulseDenominator;
    return XMVectorScale(vNormal, impulseMagnitude);
}

XMVECTOR FCollisionResponseCalculator::CalculateFrictionImpulse(
        const XMVECTOR& NormalImpulse,
        const FCollisionDetectionResult& DetectionResult,
        const FCollisionResponseParameters& ParameterA,
        const FCollisionResponseParameters& ParameterB
    )
{
    // ��� �ӵ� ��� 
    XMVECTOR vRelVel = CalculateRelativeVelocity(DetectionResult.Point, ParameterA, ParameterB);
    XMVECTOR vNormal = XMLoadFloat3(&DetectionResult.Normal);

    // ���� ���� �ӵ� ���
    XMVECTOR vNormalVel = XMVectorMultiply(vNormal,
                                           XMVector3Dot(vRelVel, vNormal));
    XMVECTOR vTangentVel = XMVectorSubtract(vRelVel, vNormalVel);

    float tangentSpeed = XMVectorGetX(XMVector3Length(vTangentVel));
    if (tangentSpeed < KINDA_SMALL)
        return XMVectorSet(0, 0, 0, 0);

    XMVECTOR vTangentDir = XMVector3Normalize(vTangentVel);

    // ������ ���
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
    // ��׶��� �¼� ���
    float velocityError = XMVectorGetX(XMVector3Dot(ConstraintCache.vRelativeVel, ConstraintCache.vNormal));
    float positionError = -DetectionResult.PenetrationDepth;
    float baumgarte = 0.05f; // ��ġ ���� ���� ���


    //��׶��� �¼� ����
    float deltaLambda = -(velocityError + baumgarte * positionError) / ConstraintCache.effectiveMass;
    float oldLambda = Accumulation.normalLambda;
    Accumulation.normalLambda = Math::Max(oldLambda + deltaLambda, 0.0f); //���� ����

    // ���� ����� ��Ÿ ����
    float appliedDelta = Accumulation.normalLambda - oldLambda;

    // �������� �� ���
    return ConstraintCache.vNormal * appliedDelta;
}

XMVECTOR FCollisionResponseCalculator::SolveFrictionConstraint(const FConstraintSolverCache& ConstraintCache,
                                                               const float StaticFriction,
                                                               const float KineticFriction,
                                                               FAccumulatedConstraint& Accumulation)
{
    // ���� ���� ��� 
    XMVECTOR vTangent = XMVector3Normalize(
        XMVectorSubtract(ConstraintCache.vRelativeVel,
                         XMVectorMultiply(ConstraintCache.vNormal,
                                          XMVector3Dot(ConstraintCache.vRelativeVel, ConstraintCache.vNormal)))
    );

    float tangentialVelocity = XMVectorGetX(XMVector3Dot(ConstraintCache.vRelativeVel, vTangent));

    // �ִ� ������ ���
    float maxFriction = std::abs(Accumulation.normalLambda) *
        (std::abs(tangentialVelocity) < KINDA_SMALL ? StaticFriction : KineticFriction);

    // ��Ÿ ���� ���
    float deltaLambda = -tangentialVelocity / ConstraintCache.effectiveMass;
    float oldLambda = Accumulation.frictionLambda;

    // ������ ����
    Accumulation.frictionLambda = std::max(-maxFriction,
                                           std::min(maxFriction, oldLambda + deltaLambda));

    // ���� ����� ��Ÿ
    float appliedDelta = Accumulation.frictionLambda - oldLambda;

    return XMVectorScale(vTangent, appliedDelta);
}

