#pragma once
#include <unordered_map>
#include <vector>
#include "CollisionDefines.h"

class UCollisionComponent;

class FCollisionEventDispatcher
{
public:
    FCollisionEventDispatcher() = default;
    ~FCollisionEventDispatcher() = default;

    // 현재 프레임의 충돌 등록
    void RegisterCollision(
        const std::shared_ptr<UCollisionComponent>& ComponentA,
        const std::shared_ptr<UCollisionComponent>& ComponentB,
        const FCollisionEventData& EventData
    );

    // 이벤트 발행 처리
    void DispatchEvents();

    // 프레임 종료 시 상태 갱신
    void UpdateCollisionStates();

    // 컴포넌트와 관련된 모든 충돌 정보를 destroyed로 마킹
    void DestroyComponent(const std::shared_ptr<UCollisionComponent>& Component);

private:
    // destroyed된 모든 컴포넌트 제거
    void RemoveDestroyedComponents();
private:
    // 충돌 상태 열거형
    enum class ECollisionState
    {
        None,
        Enter,
        Stay,
        Exit
    };

    // 충돌 정보 구조체
    struct FCollisionState
    {
        ECollisionState State = ECollisionState::None;
        FCollisionEventData EventData;
    };

private:
    // 이벤트 발행 헬퍼 함수들
    void DispatchCollisionEnter(
        const std::shared_ptr<UCollisionComponent>& ComponentA,
        const std::shared_ptr<UCollisionComponent>& ComponentB,
        const FCollisionEventData& EventData
    );

    void DispatchCollisionStay(
        const std::shared_ptr<UCollisionComponent>& ComponentA,
        const std::shared_ptr<UCollisionComponent>& ComponentB,
        const FCollisionEventData& EventData
    );

    void DispatchCollisionExit(
        const std::shared_ptr<UCollisionComponent>& ComponentA,
        const std::shared_ptr<UCollisionComponent>& ComponentB,
        const FCollisionEventData& EventData
    );

    // 컴포넌트 유효성 검사
    bool AreComponentsValid(const FCollisionPair& Key) const;

    // 충돌 쌍 키 생성
    FCollisionPair CreateCollisionKey(
        const std::shared_ptr<UCollisionComponent>& ComponentA,
        const std::shared_ptr<UCollisionComponent>& ComponentB
    ) const;

private:
    // 현재/이전 프레임의 충돌 상태 맵
    std::unordered_map<FCollisionPair, FCollisionState> CurrentCollisionsPair;
    std::unordered_map<FCollisionPair, FCollisionState> PreviousCollisionsPair;

};