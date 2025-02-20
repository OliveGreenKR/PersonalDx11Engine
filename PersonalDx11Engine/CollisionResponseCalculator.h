#pragma once
#include "Math.h"
#include "CollisionDefines.h"

class FCollisionResponseCalculator
{
public:
    // �浹 ������ ����ϴ� �� �Լ�
    FCollisionResponseResult CalculateResponse(
        const FCollisionDetectionResult& DetectionResult,
        const FCollisionResponseParameters& ParameterA,
        const FCollisionResponseParameters& ParameterB
    );

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

    //���� ���� ��� ���
    XMVECTOR CalculateContraintSolve(
        const FCollisionDetectionResult& DetectionResult,
        const FCollisionResponseParameters& ParameterA,
        const FCollisionResponseParameters& ParameterB);
};

