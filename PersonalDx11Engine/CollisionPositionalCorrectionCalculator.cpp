#include "CollisionPositionalCorrectionCalculator.h"

bool FPositionalCorrectionCalculator::CalculateMassProportionalSeparation(const FPhysicsParameters& ParmasA,
                                                               const FPhysicsParameters& ParmasB,
                                                               const FCollisionDetectionResult& CollisionResult,
                                                               Vector3& OutWorldCorrectionA, Vector3& OutWorldCorrectionB, float SafetyMargin)
{
    // 초기화: 실패 시 안전한 기본값
    OutWorldCorrectionA = Vector3::Zero();
    OutWorldCorrectionB = Vector3::Zero();

    // 입력 검증
    if (!CollisionResult.bCollided) {
        return false; // 충돌하지 않았으면 보정 불필요
    }

    if (CollisionResult.PenetrationDepth <= KINDA_SMALL) {
        return false; // 침투 깊이가 없으면 보정 불필요
    }

    // 질량 정보 추출
    float invMassA = ParmasA.InvMass;
    float invMassB = ParmasB.InvMass;
    float totalInvMass = invMassA + invMassB;

    // 정적 객체 처리
    if (totalInvMass < KINDA_SMALL) {
        return false; // 둘 다 정적이면 보정 불가능
    }

    // 질량 비례 분리 비율 계산
    float ratioA = invMassA / totalInvMass;
    float ratioB = invMassB / totalInvMass;

    // 분리 방향 (A에서 B로의 법선)
    Vector3 separationDirection = CollisionResult.Normal;

    // 분리 거리 (침투 깊이 + 안전 여유)
    float separationMagnitude = CollisionResult.PenetrationDepth + SafetyMargin;

    // 각 객체의 이동 벡터 계산
    // A는 법선의 반대 방향으로, B는 법선 방향으로 이동
    OutWorldCorrectionA = separationDirection * (-separationMagnitude * ratioA);
    OutWorldCorrectionB = separationDirection * (separationMagnitude * ratioB);

    return true;
}