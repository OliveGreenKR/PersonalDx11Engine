#include "ActorComponent.h"
#include <cassert>
#include <string>
#include <iostream>

void UActorComponent::BraodcastPostTreeInitialized()
{
    //초기화는 활성화여부와 관계업싱 동작하도록 변경
    //if (!bIsActive)
    //    return;

    // 자신의 PostInitialized 호출
    PostTreeInitialized();

    // 모든 자식 컴포넌트에 대해 PostInitialized 전파
    for (const auto& Child : ChildComponents)
    {
        if (Child)
        {
            Child->BraodcastPostTreeInitialized();
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

    // 기존 부모에서 분리
    Child->DetachFromParent();

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
    os << (isLast ? "└── " : "├── ");

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