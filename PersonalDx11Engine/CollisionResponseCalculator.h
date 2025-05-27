#pragma once
#include "Math.h"
#include "CollisionDefines.h"
#include "ConstraintInterface.h"


class IPhysicsState;

class FCollisionResponseCalculator
{
public:

    FCollisionResponseResult CalculateResponseByContraints(
        const FCollisionDetectionResult& DetectionResult,
        const FPhysicsParameters& ParameterA,
        const FPhysicsParameters& ParameterB,
        FAccumulatedConstraint& Accumulation
    );
    
    FCollisionResponseResult CalculateResponseByContraints(
        const FCollisionDetectionResult& DetectionResult,
        const FPhysicsParameters& ParameterA,
        const FPhysicsParameters& ParameterB,
        FAccumulatedConstraint& Accumulation,
        float DeltaTime);

private:

    void ClampFriction(
        const float TangentVelocityLength,
        const float NormalLambda,
        const float StaticFriction,
        const float KineticFriction,
        float& OutFrictionLambda,
        Vector3& OutTangentImpulse);

 
};

