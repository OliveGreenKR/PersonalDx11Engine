#pragma once
#include "Math.h"
#include "CollisionDefines.h"

class FCollisionResponseCalculator
{
public:
    // �浹 ������ ����ϴ� �� �Լ�
    FCollisionResponseResult CalculateResponseByImpulse(
        const FCollisionDetectionResult& DetectionResult,
        const FCollisionResponseParameters& ParameterA,
        const FCollisionResponseParameters& ParameterB
    );

    FCollisionResponseResult CalculateResponseByContraints(
        const FCollisionDetectionResult& DetectionResult,
        const FCollisionResponseParameters& ParameterA,
        const FCollisionResponseParameters& ParameterB
    );
private:
    struct FConstraintSolverCache
    {
        XMVECTOR vRelativeVel;
        XMVECTOR vNormal;
        float effectiveMass;
    };

private:
    // ��ݷ� ��� : ���� ��ݷ� ��� (ź�� �浹)
    XMVECTOR CalculateNormalImpulse(
        const FCollisionDetectionResult& DetectionResult,
        const FCollisionResponseParameters& ParameterA,
        const FCollisionResponseParameters& ParameterB
    );

    // ��ݷ� ��� : �����¿� ���� ��ݷ� ���
    XMVECTOR CalculateFrictionImpulse(
        const XMVECTOR& NormalImpulse,
        const FCollisionDetectionResult& DetectionResult,
        const FCollisionResponseParameters& ParameterA,
        const FCollisionResponseParameters& ParameterB
    );

    // ��ݷ� ��� : ��� �ӵ� ���
    XMVECTOR CalculateRelativeVelocity(
        const Vector3& ContactPoint,
        const FCollisionResponseParameters& ParameterA,
        const FCollisionResponseParameters& ParameterB
    );

    FConstraintSolverCache PrepareConstraintCache(
        const FCollisionDetectionResult& DetectionResult,
        const FCollisionResponseParameters& ParameterA,
        const FCollisionResponseParameters& ParameterB);

    //���� ���� ��� ���� �׷� 
    XMVECTOR SolveNormalConstraint(
        const FCollisionDetectionResult& DetectionResult,
        const FConstraintSolverCache& ConstraintCache,
        float& OutNormalLamda);

    //�������� ��� ������ 
    XMVECTOR SolveFrictionConstraint(
        const FConstraintSolverCache& ConstraintCache,
        const float StaticFriction,
        const float KineticFriction,
        const float NormalLamda);

 
};

