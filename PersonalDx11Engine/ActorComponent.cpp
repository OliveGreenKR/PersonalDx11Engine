#include "ActorComponent.h"
#include <cassert>

void UActorComponent::BroadcastPostitialized()
{
    // ��Ȱ��ȭ�� ��� �������� ����
    if (!bIsActive)
        return;

    // �ڽ��� PostInitialized ȣ��
    PostInitialized();

    // ��� �ڽ� ������Ʈ�� ���� PostInitialized ����
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
    // ��Ȱ��ȭ�� ��� �������� ����
    if (!bIsActive)
        return;

    // �ڽ��� Tick ȣ��
    Tick(DeltaTime);

    // ��� �ڽ� ������Ʈ�� ���� Tick ����
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
    // �ڱ� �ڽ��� �θ�� �����ϴ� �� ����
    if (InParent.get() == this)
        return;

    // ���� �θ�κ��� �и�
    DetachFromParent();

    // ���ο� �θ� ����
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

    // �ߺ� �߰� ����
    auto it = std::find_if(ChildComponents.begin(), ChildComponents.end(),
                           [&Child](const auto& Existing) {
                               return Existing.get() == Child.get();
                           });

    if (it != ChildComponents.end())
        return false;

    // ��ȯ ���� �˻�
    auto CurrentParent = GetParent();
    while (CurrentParent)
    {
        if (CurrentParent.get() == Child.get())
            return false;
        CurrentParent = CurrentParent->GetParent();
    }

    // �ڽ� ������Ʈ �߰�
    ChildComponents.push_back(Child);
    Child->ParentComponent = shared_from_this();

    // ������ ���ӿ�����Ʈ ����
    UGameObject* RootOwner = GetOwner();
    if (RootOwner)
    {
        Child->SetOwner(RootOwner);
        // �ڽ��� �ڽĵ鿡�Ե� ������ ����
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

    // �ڽ��� �θ� ���� ����
    (*it)->ParentComponent.reset();

    // �ڽ� �� �� ���� ������Ʈ���� ������ ����
    (*it)->SetOwner(nullptr);
    auto Descendants = (*it)->FindChildrenRaw<UActorComponent>();
    for (auto* Descendant : Descendants)
    {
        Descendant->SetOwner(nullptr);
    }

    // �ڽ� ������Ʈ ����
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