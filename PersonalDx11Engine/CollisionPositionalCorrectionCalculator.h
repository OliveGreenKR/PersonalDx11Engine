#pragma once
#include "Math.h"
#include "CollisionDefines.h"

class FPositionalCorrectionCalculator
{
public:
	//질량 비례 분리 계산
	bool CalculateMassProportionalSeparation(
		const FPhysicsParameters& ParmasA,
		const FPhysicsParameters& ParmasB,
		const FCollisionDetectionResult& CollisionResult,
		Vector3& OutWorldCorrectionA,
		Vector3& OutWorldCorrectionB,
		float SafetyMargin = 0.02f
	);

};

