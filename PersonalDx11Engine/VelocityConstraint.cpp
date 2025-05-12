#include "VelocityConstraint.h"
#include "CollisionDefines.h"

FVelocityConstraint::FVelocityConstraint(const Vector3& TargetDirection, float InDesireSpeed, float minLamda)
    : Direction(XMLoadFloat3(&TargetDirection)), DesiredSpeed(InDesireSpeed), MinLamda(minLamda)
{
}

FVelocityConstraint::FVelocityConstraint(const XMVECTOR& TargetDirection, float InDesiredSpeed, float minLamda)
    : Direction(TargetDirection), DesiredSpeed(InDesiredSpeed), MinLamda(minLamda)
{
}

void FVelocityConstraint::SetContactData(const Vector3& InContactPoint, const Vector3& InContactNormal, float InPenetrationDepth)
{
    ContactPoint = XMLoadFloat3(&InContactPoint);
    ContactNormal = XMLoadFloat3(&InContactNormal);
    PositionError = InPenetrationDepth;
}


Vector3 FVelocityConstraint::Solve(const FPhysicsParameters& ParameterA,
                                   const FPhysicsParameters& ParameterB, float & InOutLambda) const
{
    // 충돌 지점에서의 상대 속도 계산
    XMVECTOR RelativeVelocity = CalculateRelativeVelocity(ParameterA, ParameterB, ContactPoint);

    // 상대 속도의 해당 방향 성분 계산
    XMVECTOR DesiredRelativeVelocity = XMVector3Dot(RelativeVelocity, Direction);
    float ProjectedSpeed = XMVectorGetX(DesiredRelativeVelocity);

    // 속도 오차 계산 (현재 속도와 목표 속도의 차이)
    float VelocityError = ProjectedSpeed - DesiredSpeed;

    // 위치 오차에 따른 바이어스 항 추가 (Baumgarte 안정화)
    float PositionCorrection = Bias * PositionError;

    // 유효 질량 계산
    float EffectiveMass = CalculateEffectiveMass(ParameterA, ParameterB, ContactPoint, Direction);

    if (EffectiveMass < KINDA_SMALL)
    {
        EffectiveMass = 1.0f;  // 기본값 설정
    }

    // 람다 계산
    float DeltaLambda = -(VelocityError + PositionCorrection) / EffectiveMass;

    // 람다 누적 (제약조건에 따라 제한할 수 있음)
    float OldLambda = InOutLambda;
    InOutLambda += DeltaLambda;

    // 제약조건에 따라 람다 제한 (예: 충돌은 양수만 가능)
     InOutLambda = std::max(InOutLambda, MinLamda);

    // 실제 적용할 람다 계산
    float AppliedLambda = InOutLambda - OldLambda;
    
    // 임펄스 계산 및 반환
    XMVECTOR vResult = XMVectorScale(Direction, AppliedLambda);
    Vector3 Result;
    XMStoreFloat3(&Result, vResult);

    return Result;
}

// 유효 질량 계산 헬퍼 함수
float FVelocityConstraint::CalculateEffectiveMass(
    const FPhysicsParameters& ParameterA,
    const FPhysicsParameters& ParameterB,
    const XMVECTOR& Point,
    const XMVECTOR& Dir) const
{

    // 질량 역수
    float InvMassA = ParameterA.Mass > KINDA_SMALL ? 1.0f / ParameterA.Mass : 0.0f;
    float InvMassB = ParameterB.Mass > KINDA_SMALL ? 1.0f / ParameterB.Mass : 0.0f;

    // 회전 효과 계산
    XMVECTOR RadiusA = Point - ParameterA.Position;
    XMVECTOR RadiusB = Point - ParameterB.Position;

    XMVECTOR CrossA = XMVector3Cross(RadiusA, Dir);
    XMVECTOR CrossB = XMVector3Cross(RadiusB, Dir);

    // 회전 관성의 역행렬 계산
    Matrix RotA = XMMatrixRotationQuaternion(ParameterA.Rotation);
    Matrix RotB = XMMatrixRotationQuaternion(ParameterB.Rotation);

    XMVECTOR vInertiaA = ParameterA.RotationalInertia;
    XMVECTOR vInertiaB = ParameterB.RotationalInertia;

    XMMATRIX InertiaTensorA_W = RotA * XMMatrixScalingFromVector(vInertiaA) * XMMatrixTranspose(RotA);
    XMMATRIX InertiaTensorB_W = RotB * XMMatrixScalingFromVector(vInertiaB) * XMMatrixTranspose(RotB);

    XMMATRIX mInvInertiaA = XMMatrixInverse(nullptr, InertiaTensorA_W);
    XMMATRIX mInvInertiaB = XMMatrixInverse(nullptr, InertiaTensorB_W);

    // 회전 기여도 계산
    XMVECTOR vAngularA = XMVector3Transform(CrossA, mInvInertiaA);
    XMVECTOR vAngularB = XMVector3Transform(CrossB, mInvInertiaB);

    float AngularTermA = XMVectorGetX(XMVector3Dot(CrossA, vAngularA));
    float AngularTermB = XMVectorGetX(XMVector3Dot(CrossB, vAngularB));

    // 유효 질량 계산 (선형 + 회전)
    float EffectiveMass = InvMassA + InvMassB + AngularTermA + AngularTermB;

    // 유효 질량이 0이면 무한대로 처리 (강체)
    if (EffectiveMass < KINDA_SMALL)
        return FLT_MAX;

    return EffectiveMass;
}

// 상대 속도 계산 함수
XMVECTOR FVelocityConstraint::CalculateRelativeVelocity(
    const FPhysicsParameters& ParameterA,
    const FPhysicsParameters& ParameterB,
    const XMVECTOR& ContactPoint) const
{
    // 접촉점까지의 벡터
    XMVECTOR vRadiusA = XMVectorSubtract(ContactPoint, ParameterA.Position);
    XMVECTOR vRadiusB = XMVectorSubtract(ContactPoint, ParameterB.Position);

    // 회전에 의한 선속도
    XMVECTOR RotationalVelA = XMVector3Cross(ParameterA.AngularVelocity, vRadiusA);
    XMVECTOR RotationalVelB = XMVector3Cross(ParameterB.AngularVelocity, vRadiusB);

    // 총 속도 = 선형 속도 + 회전에 의한 속도
    XMVECTOR TotalVelA = XMVectorAdd(ParameterA.Velocity, RotationalVelA);
    XMVECTOR TotalVelB = XMVectorAdd(ParameterB.Velocity, RotationalVelB);

    // 상대 속도 = B의 속도 - A의 속도
    return TotalVelB - TotalVelA;
}
