#pragma once
#include "Math.h"
#include "CollisionDefines.h"

using namespace DirectX;

/// <summary>
/// SIMD 최적화 충돌 위치 보정 계산 시스템
/// 외부 시스템 의존성 없이 독립적 동작
/// </summary>
class FCollisionPositionCorrectionCalculator
{
#pragma region Mass Proportional Separation
public:
    /// <summary>
    /// 질량 비례 분리 계산 (SIMD 최적화)
    /// 두 객체의 역질량 비율에 따라 침투 해결을 위한 이동량 계산
    /// </summary>
    /// <param name="invMassA">객체 A의 역질량</param>
    /// <param name="invMassB">객체 B의 역질량</param>
    /// <param name="penetrationDepth">침투 깊이 (스칼라)</param>
    /// <param name="separationNormal">분리 방향 벡터 (A→B 정규화된 법선)</param>
    /// <param name="safetyMargin">안전 여유 거리</param>
    /// <param name="outCorrectionA">[출력] 객체 A 보정 벡터</param>
    /// <param name="outCorrectionB">[출력] 객체 B 보정 벡터</param>
    /// <returns>계산 성공 여부</returns>
    bool CalculateMassProportionalSeparation(
        float invMassA,
        float invMassB,
        float penetrationDepth,
        XMVECTOR separationNormal,
        float safetyMargin,
        XMVECTOR& outCorrectionA,
        XMVECTOR& outCorrectionB
    );

#pragma endregion
};