#include "VelocityConstraint.h"
#include "CollisionDefines.h"

FVelocityConstraint::FVelocityConstraint(const Vector3& TargetDirection, float InDesireSpeed, float minLambda)
    : Direction(XMLoadFloat3(&TargetDirection)), DesiredSpeed(InDesireSpeed), MinLambda(minLambda)
{
}

FVelocityConstraint::FVelocityConstraint(const XMVECTOR& TargetDirection, float InDesiredSpeed, float minLambda)
    : Direction(TargetDirection), DesiredSpeed(InDesiredSpeed), MinLambda(minLambda)
{
}

void FVelocityConstraint::SetContactData(const Vector3& InContactPoint, const Vector3& InContactNormal)
{
    ContactPoint = XMLoadFloat3(&InContactPoint);
    ContactNormal = XMLoadFloat3(&InContactNormal);;
}


Vector3 FVelocityConstraint::Solve(const FPhysicsParameters& ParameterA,
                                   const FPhysicsParameters& ParameterB, float & InOutLambda) const
{
    // 충돌 지점에서의 상대 속도 계산
    XMVECTOR RelativeVelocity = CalculateRelativeVelocity(ParameterA, ParameterB, ContactPoint);

    // 상대 속도의 해당 방향 성분 계산
    XMVECTOR DesiredRelativeSpeed = XMVector3Dot(RelativeVelocity, Direction);
    float ProjectedSpeed = XMVectorGetX(DesiredRelativeSpeed);

    // 속도 오차 계산 (현재 속도와 목표 속도의 차이)
    float VelocityError = ProjectedSpeed - DesiredSpeed;

    // 유효 질량 계산
    float EffectiveMassInv = CalculateInvEffectiveMass(ParameterA, ParameterB, ContactPoint, Direction);

    // 람다 계산
    //float DeltaLambda = -(VelocityError + PositionCorrection)  / EffectiveMassInv;
    float DeltaLambda = -(VelocityError) / EffectiveMassInv; 

    // 람다 누적 (제약조건에 따라 제한할 수 있음)
    float OldLambda = InOutLambda;
    InOutLambda += DeltaLambda;

    // 제약조건에 따라 람다 제한 (예: 충돌은 양수만 가능)
     InOutLambda = std::max(InOutLambda, MinLambda);

    // 실제 적용할 람다 계산
    float AppliedLambda = InOutLambda - OldLambda;
    
    // 임펄스 계산 및 반환
    XMVECTOR vResult = XMVectorScale(Direction, AppliedLambda);
    Vector3 Result;
    XMStoreFloat3(&Result, vResult);

    return Result;
}

// 유효 질량 계산 헬퍼 함수
float FVelocityConstraint::CalculateInvEffectiveMass(
    const FPhysicsParameters& ParameterA,
    const FPhysicsParameters& ParameterB,
    const XMVECTOR& Point,
    const XMVECTOR& Dir)
{

    // 질량 역수
    float InvMassA = ParameterA.InvMass;
    float InvMassB = ParameterB.InvMass;

    // 회전 효과 계산
    XMVECTOR RadiusA = Point - ParameterA.Position;
    XMVECTOR RadiusB = Point - ParameterB.Position;

    // 토크 방향
    XMVECTOR CrossA = XMVector3Cross(RadiusA, Dir);
    XMVECTOR CrossB = XMVector3Cross(RadiusB, Dir);

    // 회전 행렬
    Matrix RotA = XMMatrixRotationQuaternion(ParameterA.Rotation);
    Matrix RotB = XMMatrixRotationQuaternion(ParameterB.Rotation);

    XMVECTOR InvInertiaA = ParameterA.InvRotationalInertia;
    XMVECTOR InvInertiaB = ParameterB.InvRotationalInertia;

    // 월드 공간의 역 회전 관성 텐서
    XMMATRIX InertiaTensorA_W = RotA * XMMatrixScalingFromVector(InvInertiaA) * XMMatrixTranspose(RotA);
    XMMATRIX InertiaTensorB_W = RotB * XMMatrixScalingFromVector(InvInertiaB) * XMMatrixTranspose(RotB);

    // 회전 기여도 계산
    XMVECTOR vAngularA = XMVector3Transform(CrossA, InertiaTensorA_W);
    XMVECTOR vAngularB = XMVector3Transform(CrossB, InertiaTensorB_W);

    float AngularTermA = XMVectorGetX(XMVector3Dot(CrossA, vAngularA));
    float AngularTermB = XMVectorGetX(XMVector3Dot(CrossB, vAngularB));

    // 유효 질량 계산 (선형 + 회전)
    float InvEffectiveMass = InvMassA + InvMassB + AngularTermA + AngularTermB;

    // 유효 질량이 0이면 무한대로 처리 (강체)
    if (InvEffectiveMass < KINDA_SMALL)
        return 0.0f;

    return InvEffectiveMass;
}

// 상대 속도 계산 함수
XMVECTOR FVelocityConstraint::CalculateRelativeVelocity(
    const FPhysicsParameters& ParameterA,
    const FPhysicsParameters& ParameterB,
    const XMVECTOR& ContactPoint)
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
