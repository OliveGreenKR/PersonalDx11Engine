#include "ActorComponent.h"
#include <cassert>
#include <string>
#include <iostream>

void UActorComponent::BroadcastPostInitialized()
{
    //초기화는 활성화여부와 관계없이 동작
    // 자신의 PostInitialized 호출
    PostInitialized();

    // 모든 자식 컴포넌트에 대해 PostInitialized 전파
    for (const auto& Child : ChildComponents)
    {
        if (Child)
        {
            Child->BroadcastPostInitialized();
        }
    }
}

void UActorComponent::BroadcastPostTreeInitialized()
{
    //초기화는 활성화여부와 관계없이 동작
    // 자신의 PostInitialized 호출
    PostTreeInitialized();

    // 모든 자식 컴포넌트에 대해 PostInitialized 전파
    for (const auto& Child : ChildComponents)
    {
        if (Child)
        {
            Child->BroadcastPostTreeInitialized();
        }
    }
}

void UActorComponent::BroadcastTick(float DeltaTime)
{
    // 비활성화된 경우 전파하지 않음
    if (!bIsActive)
        return;

    // 자신의 Tick 호출
    Tick(DeltaTime);

    // 모든 활성화 자식 컴포넌트에 대해 Tick 전파
    for (const auto& Child : ChildComponents)
    {
        if (Child && Child->IsActive())
        {
            Child->BroadcastTick(DeltaTime);
        }
    }
}

void UActorComponent::Activate()
{
    bIsActive = true;
}

void UActorComponent::DeActivate()
{
    bIsActive = false;
}

void UActorComponent::PostInitialized()
{
}

void UActorComponent::SetParentInternal(const std::shared_ptr<UActorComponent>& InParent, bool bShouldCallEvent)
{
    // 자기 자신을 부모로 설정하는 것 방지
    if (InParent.get() == this)
        return;

    // 현재 부모와 같은 경우 무시
    auto currentParent = GetParent();
    if (currentParent.get() == InParent.get())
        return;

    // 순환 참조 검사
    if (InParent)
    {
        auto tempParent = InParent;
        while (tempParent)
        {
            if (tempParent.get() == this)
                return; // 순환 참조 발견
            tempParent = tempParent->GetParent();
        }
    }

    // 이전 부모의 자식 목록에서 제거
    if (auto oldParent = ParentComponent.lock())
    {
        auto& children = oldParent->ChildComponents;
        children.erase(
            std::remove_if(children.begin(), children.end(),
                           [this](const auto& child) { return child.get() == this; }),
            children.end());
    }

    // 새 부모 설정
    ParentComponent = InParent;

    // 새 부모의 자식 목록에 추가
    if (InParent)
    {
        InParent->ChildComponents.push_back(shared_from_this());

        // 게임 오브젝트 소유자 전파
        UGameObject* RootOwner = InParent->GetOwner();
        if (RootOwner)
        {
            SetOwner(RootOwner);
            // 자식의 자식들에게도 소유자 전파
            auto Descendants = FindChildrenRaw<UActorComponent>();
            for (auto Descendant : Descendants)
            {
                Descendant->SetOwner(RootOwner);
            }
        }
    }

    // 이벤트 호출 여부에 따라 OnParentChanged 호출
    if (bShouldCallEvent)
    {
        OnParentChanged(InParent);
    }
}

void UActorComponent::SetParent(const std::shared_ptr<UActorComponent>& InParent)
{
    SetParentInternal(InParent, true); // 이벤트 호출 포함
}

bool UActorComponent::AddChild(const std::shared_ptr<UActorComponent>& Child)
{
    if (!Child || Child.get() == this)
        return false;

    // 중복 추가 방지
    auto it = std::find_if(ChildComponents.begin(), ChildComponents.end(),
                           [&Child](const auto& Existing) {
                               return Existing.get() == Child.get();
                           });

    if (it != ChildComponents.end())
        return false;

    // SetParentInternal을 통해 계층 구조 설정 (이벤트 호출 포함)
    Child->SetParentInternal(shared_from_this(), true);
    return true;
}

void UActorComponent::DetachFromParent()
{
    SetParentInternal(nullptr, true);
}

bool UActorComponent::RemoveChild(const std::shared_ptr<UActorComponent>& Child)
{
    if (!Child)
        return false;

    // 자식 검색
    auto it = std::find_if(ChildComponents.begin(), ChildComponents.end(),
                           [&Child](const auto& Existing) {
                               return Existing.get() == Child.get();
                           });

    // 자식이 없으면 종료
    if (it == ChildComponents.end())
        return false;

    // 자식의 부모 제거
    Child->DetachFromParent();

    return true;
}

UActorComponent* UActorComponent::GetRoot() const
{
    const UActorComponent* Current = this;
    while (auto Parent = Current->GetParent())
    {
        Current = Parent.get();
    }
    return const_cast<UActorComponent*>(Current);
}

void UActorComponent::PrintComponentTree(std::ostream& os) const
{
    // 루트 컴포넌트부터 트리 출력 시작
    PrintComponentTreeInternal(os, "", true);
}

void UActorComponent::PrintComponentTreeInternal(std::ostream& os, std::string prefix, bool isLast) const
{
    // 현재 컴포넌트 출력 라인
    os << prefix;

    // 마지막 자식인지 여부에 따라 다른 접두사 사용
    os << (isLast ? "+-- " : "|-- ");

    // 컴포넌트 이름 출력
    os << GetComponentClassName() << std::endl;

    // 다음 자식들을 위한 새 접두사 계산
    std::string newPrefix = prefix + (isLast ? "    " : "│   ");

    // 자식들이 있는 경우 출력
    if (!ChildComponents.empty())
    {
        // 마지막 자식 인덱스 계산 
        size_t lastValidIndex = ChildComponents.size() - 1;
        
        //유효자식만 카운트
        //while (lastValidIndex > 0 && !ChildComponents[lastValidIndex])
        //{
        //    --lastValidIndex;
        //}

        // 각 자식 컴포넌트 출력
        for (size_t i = 0; i < ChildComponents.size(); ++i)
        {
            if (ChildComponents[i])
            {
                bool isChildLast = (i >= lastValidIndex);
                ChildComponents[i]->PrintComponentTreeInternal(os, newPrefix, isChildLast);
            }
            else
            {
                os << "└── " << "ERRORCHILD" << std::endl;
            }
        }
    }
}