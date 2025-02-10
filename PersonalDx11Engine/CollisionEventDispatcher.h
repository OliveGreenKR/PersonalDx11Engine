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

    // ���� �������� �浹 ���
    void RegisterCollision(
        const std::shared_ptr<UCollisionComponent>& ComponentA,
        const std::shared_ptr<UCollisionComponent>& ComponentB,
        const FCollisionEventData& EventData
    );

    // �̺�Ʈ ���� ó��
    void DispatchEvents();

    // ������ ���� �� ���� ����
    void UpdateCollisionStates();

    // ������Ʈ�� ���õ� ��� �浹 ������ destroyed�� ��ŷ
    void DestroyComponent(const std::shared_ptr<UCollisionComponent>& Component);

private:
    // destroyed�� ��� ������Ʈ ����
    void RemoveDestroyedComponents();
private:
    // �浹 ���� ������
    enum class ECollisionState
    {
        None,
        Enter,
        Stay,
        Exit
    };

    // �浹 ���� ����ü
    struct FCollisionState
    {
        ECollisionState State = ECollisionState::None;
        FCollisionEventData EventData;
    };

private:
    // �̺�Ʈ ���� ���� �Լ���
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

    // ������Ʈ ��ȿ�� �˻�
    bool AreComponentsValid(const FCollisionPair& Key) const;

    // �浹 �� Ű ����
    FCollisionPair CreateCollisionKey(
        const std::shared_ptr<UCollisionComponent>& ComponentA,
        const std::shared_ptr<UCollisionComponent>& ComponentB
    ) const;

private:
    // ����/���� �������� �浹 ���� ��
    std::unordered_map<FCollisionPair, FCollisionState> CurrentCollisionsPair;
    std::unordered_map<FCollisionPair, FCollisionState> PreviousCollisionsPair;

};