#include "CollisionResponseCalculator.h"

FCollisionResponseResult FCollisionResponseCalculator::CalculateResponse(
    const FCollisionDetectionResult& DetectionResult,
    const FCollisionResponseParameters& ParameterA,
    const FCollisionResponseParameters& ParameterB)
{
     FCollisionResponseResult ResponseData;

    if (!DetectionResult.bCollided || ParameterA.Mass < 0.0f || ParameterB.Mass < 0.0f )
        return ResponseData;

    // �������� ���� ���� �ε�
    ResponseData.ApplicationPoint = DetectionResult.Point;
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


XMVECTOR FCollisionResponseCalculator::CalculateContraintSolve(const FCollisionDetectionResult& DetectionResult, const FCollisionResponseParameters& ParameterA, const FCollisionResponseParameters& ParameterB)
{
    //struct FContactPoint
    //{
    //    Vector3 Position;        // ������ ��ġ
    //    Vector3 Normal;         // ���˸� �븻
    //    float Penetration;      // ħ�� ����
    //    float AccumulatedNormalImpulse;   // ������ ���� ��ݷ�
    //    float AccumulatedTangentImpulse;  // ������ ���� ��ݷ�
    //};

    XMVECTOR vContactPoint = XMLoadFloat3(&DetectionResult.Point);
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

    // 1. ��� �ӵ� ���
    XMVECTOR vRelativeVel = XMVectorSubtract(vTotalVelB, vTotalVelA);

    // 2. J ���
    //J = [-n, -(r �� n), n, (r �� n)]
    XMVECTOR vNormal = XMLoadFloat3(&DetectionResult.Normal);
    XMMATRIX J;

    XMVECTOR vJA_Angular = XMVector3Cross(vRadiusA, vNormal);
    XMVECTOR vJB_Angular = XMVector3Cross(vRadiusB, vNormal);

    J.r[0] = -vNormal;                          //JA_Linear  - ������
    J.r[1] = vJA_Angular;                        //JA_Angular - ���ӵ������� ����
    J.r[2] = vNormal;                           //JB_Linear  
    J.r[3] = vJB_Angular;                       //JB_Angular

    float invMassA = ParameterA.Mass > KINDA_SMALL ? 1.0f / ParameterA.Mass : 0.0f;
    float invMassB = ParameterB.Mass > KINDA_SMALL ? 1.0f / ParameterB.Mass : 0.0f;

    // ȸ�� ���� ���
    Matrix RotA = XMMatrixRotationQuaternion(XMLoadFloat4(&ParameterA.Rotation));
    Matrix RotB = XMMatrixRotationQuaternion(XMLoadFloat4(&ParameterB.Rotation));

    XMVECTOR vInertiaA = XMLoadFloat3(&ParameterA.RotationalInertia);
    XMVECTOR vInertiaB = XMLoadFloat3(&ParameterB.RotationalInertia);

    // ���� ���� ȸ�� ���� �ټ�
    XMMATRIX InertiaTensorA_W = RotA * XMMatrixScalingFromVector(vInertiaA) * XMMatrixTranspose(RotA);
    XMMATRIX InertiaTensorB_W = RotB * XMMatrixScalingFromVector(vInertiaB) * XMMatrixTranspose(RotB);

    // ȸ�� ���� ����� ���
    XMMATRIX mInvInertiaA = XMMatrixInverse(nullptr, InertiaTensorA_W);
    XMMATRIX mInvInertiaB = XMMatrixInverse(nullptr, InertiaTensorB_W);

    XMVECTOR JA_Angular_Inertia = XMVector3Transform(vJA_Angular, mInvInertiaA);
    XMVECTOR JB_Angular_Inertia = XMVector3Transform(vJB_Angular, mInvInertiaB);

    // 3. ��ȿ���� ���
    float effectiveMass =
        invMassA + invMassB +
        XMVectorGetX(XMVector3Dot(vJA_Angular, JA_Angular_Inertia)) +
        XMVectorGetX(XMVector3Dot(vJB_Angular, JB_Angular_Inertia));


    // 4. ��׶��� �¼� ���
    float velocityError = XMVectorGetX(XMVector3Dot(vRelativeVel, vNormal));
    float positionError = -DetectionResult.PenetrationDepth;
    float baumgarte = 0.1f; // ��ġ ���� ���� ���

    float lambda = -(velocityError + baumgarte * positionError) / effectiveMass;

    //// 5. �������� ���� ���� (��ħ�� ����)
    //float oldAccumulatedLambda = contact.accumulatedLambda;
    //contact.accumulatedLambda = Max(0.0f, oldAccumulatedLambda + lambda);
    //lambda = contact.accumulatedLambda - oldAccumulatedLambda;

    // 6. �������� �� ���
    return vNormal * lambda;
}

