#pragma once
#include "ConstraintInterface.h"
#include "Math.h"


struct FPhysicsParameters;

// 속도 제약조건 (다양한 목적으로 재사용 가능)
class FVelocityConstraint : public IConstraint
{
private:
    XMVECTOR Direction = XMVectorZero();        // 제약 방향 
    float DesiredSpeed = 0.0f;    // 목표 속도 
    float Bias = 0.0f;               // 위치 오차 보정 계수
    float PositionError = 0.0f;      // 위치 오차 
    float MinLamda = FLT_MIN;        //Lamda 제약
    XMVECTOR ContactPoint = XMVectorZero();
    XMVECTOR ContactNormal = XMVectorZero();

public:
    FVelocityConstraint(const Vector3& TargetDirection, float InDesiredSpeed = 0.0f, float minLamda = FLT_MIN);
    FVelocityConstraint(const XMVECTOR& TargetDirection, float InDesiredSpeed = 0.0f, float minLamda = FLT_MIN);
    // IConstraint 구현
    virtual Vector3 Solve(const FPhysicsParameters& ParameterA,
                          const FPhysicsParameters& ParameterB, float & InOutLambda) const override;

    

    virtual void SetContactData(const Vector3& InContactPoint,
                                const Vector3& InContactNormal,
                                float InPenetrationDepth) override;

    // 위치 오차 보정용 설정
    void SetBias(float InBias) { Bias = InBias; }
    // 목표 속도 설정
    void SetDesiredSpeed(float InDesiredSpeed) { DesiredSpeed = InDesiredSpeed; }

    float CalculateEffectiveMass(const FPhysicsParameters& ParameterA, const FPhysicsParameters& ParameterB,
                                 const XMVECTOR& Point, const XMVECTOR& Dir) const;

    XMVECTOR CalculateRelativeVelocity (const FPhysicsParameters& BodyA, const FPhysicsParameters& BodyB,
                                       const XMVECTOR& ContactPoint) const;
};

