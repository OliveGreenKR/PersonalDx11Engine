#pragma once
// 물리 시스템 클래스 (개념적 설계)
#include <vector>
#include "PhysicsStateInterface.h"
#include <memory>


class FCollisionProcessor;

class PhysicsSystem
{
private:
    // 등록된 물리 객체들
    std::vector<std::weak_ptr<IPhysicsState>> RegisteredObjects;

    // 충돌 관리자
    FCollisionProcessor* CollisionProcessor;

    // 물리 시뮬레이션 설정
    float FixedTimeStep = 0.016f;  // 60Hz
    float AccumulatedTime = 0.0f;
    int MaxSubsteps = 3;

    // 시뮬레이션 상태
    bool bIsSimulating = false;

public:
    // 물리 객체 등록/해제
    void RegisterPhysicsObject(std::shared_ptr<IPhysicsState>& Object);
    void UnregisterPhysicsObject(std::shared_ptr<IPhysicsState>& Object);

    // 메인 물리 업데이트 (게임 루프에서 호출)
    void TickPhysics(const float DeltaTime);

private:
    // 필요한 서브스텝 수 계산
    int CalculateRequiredSubsteps();

    // 시뮬레이션 시작 전 준비
    void PrepareSimulation();

    // 단일 서브스텝 시뮬레이션
    bool SimulateSubstep(const float TimeStep);

    // 시뮬레이션 완료 후 최종 상태 적용
    void FinalizeState();
};