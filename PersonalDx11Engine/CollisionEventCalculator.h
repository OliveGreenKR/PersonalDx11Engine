#pragma once
#include "Math.h"
#include "CollisionDefines.h"

using namespace DirectX;
using PhysicsID = std::uint32_t;

/// <summary>
/// 순수 충돌 이벤트 생성 시스템
/// 외부에서 제공받은 데이터로 FPhysicsCollisionEvent 생성만 담당
/// 모든 외부 의존성 제거, 순수 계산 함수들로 구성
/// </summary>
class FCollisionEventCalculator
{
#pragma region Core Event Generation
public:
    /// <summary>
    /// 충돌 검출 결과를 바탕으로 물리 이벤트 생성
    /// CollisionProcessor에서 모든 필요한 데이터를 제공받아 순수 변환만 수행
    /// </summary>
    /// <param name="DetectResult">현재 프레임 충돌 검출 결과</param>
    /// <param name="bPrevCollided">이전 프레임 충돌 여부</param>
    /// <param name="PhysicsIdA">물체 A의 PhysicsID</param>
    /// <param name="PhysicsIdB">물체 B의 PhysicsID</param>
    /// <returns>생성된 물리 이벤트</returns>
    FPhysicsCollisionEvent GenerateCollisionEvent(const FCollisionDetectionResult& DetectResult,
                                                  bool bPrevCollided,
                                                  PhysicsID PhysicsIdA,
                                                  PhysicsID PhysicsIdB
    );

    /// <summary>
    /// Exit 이벤트 생성
    /// 이전에 충돌했지만 현재 충돌하지 않는 경우용
    /// </summary>
    /// <param name="PhysicsIdA">물체 A ID</param>
    /// <param name="PhysicsIdB">물체 B ID</param>
    /// <returns>Exit 상태의 물리 이벤트</returns>
    FPhysicsCollisionEvent GenerateExitEvent(
        PhysicsID PhysicsIdA,
        PhysicsID PhysicsIdB
    );

#pragma endregion

#pragma region Event State Determination
public:
    /// <summary>
    /// 충돌 상태 결정 (Enter/Stay/Exit)
    /// 현재 충돌 여부와 이전 충돌 여부를 바탕으로 상태 결정
    /// </summary>
    /// <param name="bCurrentlyColliding">현재 충돌 여부</param>
    /// <param name="bPreviouslyColliding">이전 충돌 여부</param>
    /// <returns>충돌 상태</returns>
    ECollisionState DetermineCollisionState(
        bool bCurrentlyColliding,
        bool bPreviouslyColliding
    );

#pragma endregion
};