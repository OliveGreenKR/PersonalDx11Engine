#include "CollisionPositionalCorrectionCalculator.h"

#pragma region Mass Proportional Separation

bool FCollisionPositionCorrectionCalculator::CalculateMassProportionalSeparation(
    float invMassA,
    float invMassB,
    float penetrationDepth,
    XMVECTOR separationNormal,
    float safetyMargin,
    XMVECTOR& outCorrectionA,
    XMVECTOR& outCorrectionB)
{
    // 초기화: 실패 시 안전한 기본값
    outCorrectionA = XMVectorZero();
    outCorrectionB = XMVectorZero();

    // 입력 유효성 검증
    if (penetrationDepth <= KINDA_SMALL)
    {
        return false; // 침투 깊이가 없으면 보정 불필요
    }

    // 총 역질량 계산
    float totalInvMass = invMassA + invMassB;
    if (totalInvMass < KINDA_SMALL)
    {
        return false; // 둘 다 정적 객체면 보정 불가능
    }

    // 질량 비례 분리 비율 계산 (SIMD 최적화)
    XMVECTOR invMassVector = XMVectorSet(invMassA, invMassB, totalInvMass, 0.0f);
    XMVECTOR ratioVector = XMVectorDivide(invMassVector, XMVectorSplatZ(invMassVector));

    float ratioA = XMVectorGetX(ratioVector);
    float ratioB = XMVectorGetY(ratioVector);

    // 총 분리 거리 계산
    float totalSeparationDistance = penetrationDepth + safetyMargin;
    XMVECTOR separationDistanceVector = XMVectorReplicate(totalSeparationDistance);

    // 각 객체의 이동 벡터 계산 (SIMD 최적화)
    // A는 법선의 반대 방향으로, B는 법선 방향으로 이동
    XMVECTOR ratioAVector = XMVectorReplicate(-ratioA);
    XMVECTOR ratioBVector = XMVectorReplicate(ratioB);

    outCorrectionA = XMVectorMultiply(
        XMVectorMultiply(separationNormal, separationDistanceVector),
        ratioAVector
    );

    outCorrectionB = XMVectorMultiply(
        XMVectorMultiply(separationNormal, separationDistanceVector),
        ratioBVector
    );

    return true;
}

#pragma endregion