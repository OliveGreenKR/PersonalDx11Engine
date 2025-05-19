#pragma once
// 물리 시스템 클래스 (개념적 설계)
#include <vector>
#include "PhysicsObjectInterface.h"
#include <memory>

class FCollisionProcessorT;

class UPhysicsSystem
{
private:
    UPhysicsSystem();
    ~UPhysicsSystem();

    // 복사 및 이동 방지
    UPhysicsSystem(const UPhysicsSystem&) = delete;
    UPhysicsSystem& operator=(const UPhysicsSystem&) = delete;
    UPhysicsSystem(UPhysicsSystem&&) = delete;
    UPhysicsSystem& operator=(UPhysicsSystem&&) = delete;
private:
    // 등록된 물리 객체들
    std::vector<std::weak_ptr<IPhysicsObejct>> RegisteredObjects;

    // 하부시스템
        //충돌
    FCollisionProcessorT* CollisionProcessor;

public:
    static UPhysicsSystem* Get()
    {
        static UPhysicsSystem* manager = [](){
            UPhysicsSystem* instance = new UPhysicsSystem();
            return instance;
        }();

        return manager;
    }
    // 물리 객체 등록/해제
    void RegisterPhysicsObject(std::shared_ptr<IPhysicsObejct>& Object);
    void UnregisterPhysicsObject(std::shared_ptr<IPhysicsObejct>& Object);

    // 메인 물리 업데이트 (게임 루프에서 호출)
    void TickPhysics(const float DeltaTime);


#pragma region Debug
    void PrintDebugInfo();
#pragma endregion
private:
    // 필요한 서브스텝 수 계산
    int CalculateRequiredSubsteps();

    // 시뮬레이션 시작 전 준비
    void PrepareSimulation();

    // 단일 서브스텝 시뮬레이션
    bool SimulateSubstep(const float TimeStep);

    // 시뮬레이션 완료 후 최종 상태 적용
    void FinalizeSimulation();

private:
    // 물리 시뮬레이션 설정
    float FixedTimeStep = 0.016f;  // 60Hz
    float AccumulatedTime = 0.0f;
    int MaxSubsteps = 3;

    // 시뮬레이션 상태
    bool bIsSimulating = false;



};