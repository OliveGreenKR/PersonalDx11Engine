#pragma once
#include <memory>
#include <vector>
#include <algorithm>

class UGameObject;

class UActorComponent : public std::enable_shared_from_this<UActorComponent>
{
public:
    UActorComponent() = default;
    virtual ~UActorComponent() = default;

    // ���� ���� �Լ�
    virtual void PostInitialize() {}
    virtual void Tick(float DeltaTime) {}
    virtual UGameObject* GetOwner() const { return ParentComponent.expired() ? OwnerObject : GetRoot()->OwnerObject; }

protected:
    // ���� ���� ����
    void SetOwner(UGameObject* InOwner) { OwnerObject = InOwner; }
    void SetParent(const std::shared_ptr<UActorComponent>& InParent);

    // ������Ʈ ���� ���� ����
    bool AddChild(const std::shared_ptr<UActorComponent>& Child);
    bool RemoveChild(const std::shared_ptr<UActorComponent>& Child);
    void DetachFromParent();

    // ���� ���� Ž��
    UActorComponent* GetRoot() const;
    std::shared_ptr<UActorComponent> GetParent() const { return ParentComponent.lock(); }
    const std::vector<std::shared_ptr<UActorComponent>>& GetChildren() const { return ChildComponents; }

    // ������Ʈ �˻� ��ƿ��Ƽ
    template<typename T>
    T* FindComponentByType() const
    {
        // ���� ������Ʈ üũ
        if (auto ThisComponent = dynamic_cast<T*>(this))
            return ThisComponent;

        // �ڽ� ������Ʈ�� �˻�
        for (const auto& Child : ChildComponents)
        {
            if (auto FoundComponent = Child->FindComponentByType<T>())
                return FoundComponent;
        }

        return nullptr;
    }

    template<typename T>
    std::vector<T*> FindComponentsByType() const
    {
        std::vector<T*> Found;

        // ���� ������Ʈ üũ
        if (auto ThisComponent = dynamic_cast<T*>(this))
            Found.push_back(ThisComponent);

        // �ڽ� ������Ʈ�� �˻�
        for (const auto& Child : ChildComponents)
        {
            auto ChildComponents = Child->FindComponentsByType<T>();
            Found.insert(Found.end(), ChildComponents.begin(), ChildComponents.end());
        }

        return Found;
    }

private:
    UGameObject* OwnerObject = nullptr;
    std::weak_ptr<UActorComponent> ParentComponent;
    std::vector<std::shared_ptr<UActorComponent>> ChildComponents;

    friend class UGameObject;
};