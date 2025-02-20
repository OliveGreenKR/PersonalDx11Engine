#include "CollisionResponseCalculator.h"

FCollisionResponseResult FCollisionResponseCalculator::CalculateResponse(
    const FCollisionDetectionResult& DetectionResult,
    const FCollisionResponseParameters& ParameterA,
    const FCollisionResponseParameters& ParameterB)
{
     FCollisionResponseResult ResponseData;

    if (!DetectionResult.bCollided || ParameterA.Mass < 0.0f || ParameterB.Mass < 0.0f )
        return ResponseData;

    // 접촉점과 법선 벡터 로드
    ResponseData.ApplicationPoint = DetectionResult.Point;
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

XMVECTOR FCollisionResponseCalculator::CalculateNormalImpulse(
    const FCollisionDetectionResult& DetectionResult,
    const FCollisionResponseParameters& ParameterA,
    const FCollisionResponseParameters& ParameterB)
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
        const FCollisionResponseParameters& ParameterA,
        const FCollisionResponseParameters& ParameterB
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


XMVECTOR FCollisionResponseCalculator::CalculateContraintSolve(const FCollisionDetectionResult& DetectionResult, const FCollisionResponseParameters& ParameterA, const FCollisionResponseParameters& ParameterB)
{
    struct FContactPoint
    {
        Vector3 Position;        // 접촉점 위치
        Vector3 Normal;         // 접촉면 노말
        float Penetration;      // 침투 깊이
        float AccumulatedNormalImpulse;   // 누적된 수직 충격량
        float AccumulatedTangentImpulse;  // 누적된 접선 충격량
    };

    return XMVECTOR();
}

