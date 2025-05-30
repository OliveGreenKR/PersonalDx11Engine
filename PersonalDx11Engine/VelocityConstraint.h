#pragma once
#include "ConstraintInterface.h"
#include "Math.h"


struct FPhysicsParameters;

// 속도 제약조건 (다양한 목적으로 재사용 가능)
class FVelocityConstraint : public IConstraint
{
private:
    XMVECTOR Direction = XMVectorZero();        // 제약 방향 
    float DesiredSpeed = 0.0f;                 // 목표 속도 
    float MinLambda = -FLT_MAX;                //Lamda 제약
    XMVECTOR ContactPoint = XMVectorZero();
    XMVECTOR ContactNormal = XMVectorZero();

public:
    FVelocityConstraint(const Vector3& TargetDirection, float InDesiredSpeed = 0.0f, float minLambda = -FLT_MAX);
    FVelocityConstraint(const XMVECTOR& TargetDirection, float InDesiredSpeed = 0.0f, float minLambda = -FLT_MAX);
    // IConstraint 구현
    virtual Vector3 Solve(const FPhysicsParameters& ParameterA,
                          const FPhysicsParameters& ParameterB, float & InOutLambda) const override;

    

    virtual void SetContactData(const Vector3& InContactPoint,
                                const Vector3& InContactNormal) override;

    // 목표 속도 설정
    void SetDesiredSpeed(float InDesiredSpeed) { DesiredSpeed = InDesiredSpeed; }

    static float CalculateInvEffectiveMass(const FPhysicsParameters& ParameterA, const FPhysicsParameters& ParameterB,
                                 const XMVECTOR& Point, const XMVECTOR& Dir);

    static XMVECTOR CalculateRelativeVelocity (const FPhysicsParameters& BodyA, const FPhysicsParameters& BodyB,
                                       const XMVECTOR& ContactPoint);
};

