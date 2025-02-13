#pragma once
#include <memory>
#include <vector>
#include <algorithm>
#include <queue>

class UGameObject;

class UActorComponent : public std::enable_shared_from_this<UActorComponent>
{
public:
    UActorComponent() = default;
    virtual ~UActorComponent() = default;


    UGameObject* GetOwner() const { return ParentComponent.expired() ? OwnerObject : GetRoot()->OwnerObject; }


public:
    // �ʱ�ȭ ����
    void BroadcastPostInitializedForComponents();

    // Tick ����
    void BroadcastTick(float DeltaTime);

    // ������Ʈ Ȱ��ȭ ����
    void SetActive(bool bNewActive) { bIsActive = bNewActive; }
    bool IsActive() const { return bIsActive; }

protected:
    // ���� ���� �Լ� - root�� ���ؼ� ���ĵ� ����, ���� �ʱ�ȭ ���� ȣ��Ǿ����
    virtual void PostInitialized() {}
    virtual void Tick(float DeltaTime) {}
    // ���� ���� ����
    void SetOwner(UGameObject* InOwner) { OwnerObject = InOwner; }
    void SetParent(const std::shared_ptr<UActorComponent>& InParent);

public:
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
    T* FindChildByType() const
    {
        // �ڽ� ������Ʈ�� �˻�
        for (const auto& Child : ChildComponents)
        {
            if (auto FoundComponent = Child->FindComponentByType<T>())
                return FoundComponent;
        }

        return nullptr;
    }

    template<typename T>
    std::vector<std::weak_ptr<T>> FindChildrenByType() const
    {
        std::vector<std::weak_ptr<T>> Found;

        // �ڽ� ������Ʈ�� �˻�
        for (const auto& Child : ChildComponents)
        {
            if (Child)
            {
                auto ChildComponents = Child->FindComponentsByType<T>();
                Found.insert(Found.end(), ChildComponents.begin(), ChildComponents.end());
            }
        }

        return Found;
    }

    template<typename T>
    std::vector<std::weak_ptr<T>> FindComponentsByType() const
    {
        std::vector<std::weak_ptr<T>> Found;

        std::shared_ptr<UActorComponent>& SharedThis = shared_from_this();
        Found.push_back(SharedThis);


        // �ڽ� ������Ʈ�� �˻�
        for (const auto& Child : ChildComponents)
        {
            if (Child)
            {
                auto ChildComponents = Child->FindComponentsByType<T>();
                Found.insert(Found.end(), ChildComponents.begin(), ChildComponents.end());
            }
        }

        return Found;
    }

private:
    // non-const ������ raw ������ ��ȯ �Լ��� �Բ� ����
    template<typename T>
    std::vector<T*> FindComponentsRaw() const
    {
        std::vector<T*> Found;

        // ���� ������Ʈ üũ
        if (auto ThisComponent = dynamic_cast<const T*>(this))
        {
            Found.push_back(const_cast<T*>(ThisComponent));
        }

        // �ڽ� ������Ʈ�� �˻�
        for (const auto& Child : ChildComponents)
        {
            if (Child)
            {
                auto ChildComponents = Child->FindComponentsRaw<T>();
                Found.insert(Found.end(), ChildComponents.begin(), ChildComponents.end());
            }
        }

        return Found;
    }

    // non-const ������ raw ������ ��ȯ �Լ��� �Բ� ����
    template<typename T>
    std::vector<T*> FindChildrenRaw() const
    {
        std::vector<T*> Found;

        // �ڽ� ������Ʈ�� �˻�
        for (const auto& Child : ChildComponents)
        {
            if (Child)
            {
                auto ChildComponents = Child->FindComponentsRaw<T>();
                Found.insert(Found.end(), ChildComponents.begin(), ChildComponents.end());
            }
        }

        return Found;
    }

private:
    bool bIsActive = true;

    //�����ֱⰡ ���ӵɰ��̱⿡ �Ϲ� raw ���
    UGameObject* OwnerObject = nullptr;
    std::weak_ptr<UActorComponent> ParentComponent;
    std::vector<std::shared_ptr<UActorComponent>> ChildComponents;

    friend class UGameObject;
};