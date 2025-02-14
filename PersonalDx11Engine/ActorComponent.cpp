#include "ActorComponent.h"
#include <cassert>

void UActorComponent::BroadcastPostitialized()
{
    // 비활성화된 경우 전파하지 않음
    if (!bIsActive)
        return;

    // 자신의 PostInitialized 호출
    PostInitialized();

    // 모든 자식 컴포넌트에 대해 PostInitialized 전파
    for (const auto& Child : ChildComponents)
    {
        if (Child)
        {
            Child->BroadcastPostitialized();
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

    // 모든 자식 컴포넌트에 대해 Tick 전파
    for (const auto& Child : ChildComponents)
    {
        if (Child)
        {
            Child->BroadcastTick(DeltaTime);
        }
    }
}

void UActorComponent::SetParent(const std::shared_ptr<UActorComponent>& InParent)
{
    // 자기 자신을 부모로 설정하는 것 방지
    if (InParent.get() == this)
        return;

    // 이전 부모로부터 분리
    DetachFromParent();

    // 새로운 부모에 연결
    if (InParent)
    {
        ParentComponent = InParent;
        InParent->AddChild(shared_from_this());
    }
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

    // 순환 참조 검사
    auto CurrentParent = GetParent();
    while (CurrentParent)
    {
        if (CurrentParent.get() == Child.get())
            return false;
        CurrentParent = CurrentParent->GetParent();
    }

    // 자식 컴포넌트 추가
    ChildComponents.push_back(Child);
    Child->ParentComponent = shared_from_this();

    // 소유자 게임오브젝트 전파
    UGameObject* RootOwner = GetOwner();
    if (RootOwner)
    {
        Child->SetOwner(RootOwner);
        // 자식의 자식들에게도 소유자 전파
        auto Descendants = Child->FindChildrenRaw<UActorComponent>();
        for (auto Descendant : Descendants)
        {
            Descendant->SetOwner(RootOwner);
        }
    }

    return true;
}

bool UActorComponent::RemoveChild(const std::shared_ptr<UActorComponent>& Child)
{
    if (!Child)
        return false;

    auto it = std::find_if(ChildComponents.begin(), ChildComponents.end(),
                           [&Child](const auto& Existing) {
                               return Existing.get() == Child.get();
                           });

    if (it == ChildComponents.end())
        return false;

    // 자식의 부모 참조 제거
    (*it)->ParentComponent.reset();

    // 자식 및 그 하위 컴포넌트들의 소유자 제거
    (*it)->SetOwner(nullptr);
    auto Descendants = (*it)->FindChildrenRaw<UActorComponent>();
    for (auto* Descendant : Descendants)
    {
        Descendant->SetOwner(nullptr);
    }

    // 자식 컴포넌트 제거
    ChildComponents.erase(it);
    return true;
}

void UActorComponent::DetachFromParent()
{
    if (auto Parent = ParentComponent.lock())
    {
        Parent->RemoveChild(shared_from_this());
    }
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