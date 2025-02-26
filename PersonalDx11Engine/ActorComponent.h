#pragma once
#include <memory>
#include <vector>
#include <algorithm>
#include <queue>

class UGameObject;

class UActorComponent : public std::enable_shared_from_this<UActorComponent>
{

public:
    UActorComponent() : bIsActive(true) {}
    virtual ~UActorComponent() = default;

    UGameObject* GetOwner() const { return ParentComponent.expired() ? OwnerObject : GetRoot()->OwnerObject; }


    template<typename T, typename ...Args>
    static std::shared_ptr<T> Create(Args&&... args)
    {
        // T�� Base�� ��ӹ޾Ҵ��� ������ Ÿ�ӿ� üũ
        static_assert(std::is_base_of_v<UActorComponent, T> || std::is_same_v<T, UActorComponent>,
                      "T must inherit from Base");
        // std::make_shared�� ����Ͽ� ��ü ����
        return std::make_shared<T>(std::forward<Args>(args)...);
    }

private:
    // private ����/�̵� �����ڿ� ���� ������
    UActorComponent(const UActorComponent&) = delete;
    UActorComponent& operator=(const UActorComponent&) = delete;
    UActorComponent(UActorComponent&&) = delete;
    UActorComponent& operator=(UActorComponent&&) = delete;

public:
    void BraodcastPostTreeInitialized();

    // Tick ����
    void BroadcastTick(float DeltaTime);

    virtual bool IsEffective() { return  this != nullptr && IsActive(); }

    // ������Ʈ Ȱ��ȭ ����
    void SetActive(bool bNewActive) { bIsActive = bNewActive; }
    bool IsActive() const { return bIsActive; }

protected:
    // ������Ʈ Ʈ�� ���� �ϼ� �� �ʱ�ȭ(���� �� Ʈ������ ���� ����)
    virtual void PostTreeInitialized(){}

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
    T* FindComponentByType()
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
    T* FindChildByType()
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
    std::vector<std::weak_ptr<T>> FindChildrenByType()
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
    std::vector<std::weak_ptr<T>> FindComponentsByType()
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
    std::vector<T*> FindComponentsRaw()
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
    std::vector<T*> FindChildrenRaw()
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
    bool bIsActive : 1;

    //�����ֱⰡ ���ӵɰ��̱⿡ �Ϲ� raw ���
    UGameObject* OwnerObject = nullptr;
    std::weak_ptr<UActorComponent> ParentComponent;
    std::vector<std::shared_ptr<UActorComponent>> ChildComponents;

    friend class UGameObject;
};