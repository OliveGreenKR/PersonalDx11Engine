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

    // 순수 가상 함수
    virtual void PostInitialize() {}
    virtual void Tick(float DeltaTime) {}
    virtual UGameObject* GetOwner() const { return ParentComponent.expired() ? OwnerObject : GetRoot()->OwnerObject; }

protected:
    // 소유 관계 설정
    void SetOwner(UGameObject* InOwner) { OwnerObject = InOwner; }
    void SetParent(const std::shared_ptr<UActorComponent>& InParent);

    // 컴포넌트 계층 구조 관리
    bool AddChild(const std::shared_ptr<UActorComponent>& Child);
    bool RemoveChild(const std::shared_ptr<UActorComponent>& Child);
    void DetachFromParent();

    // 계층 구조 탐색
    UActorComponent* GetRoot() const;
    std::shared_ptr<UActorComponent> GetParent() const { return ParentComponent.lock(); }
    const std::vector<std::shared_ptr<UActorComponent>>& GetChildren() const { return ChildComponents; }

    // 컴포넌트 검색 유틸리티
    template<typename T>
    T* FindComponentByType() const
    {
        // 현재 컴포넌트 체크
        if (auto ThisComponent = dynamic_cast<T*>(this))
            return ThisComponent;

        // 자식 컴포넌트들 검색
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

        // 현재 컴포넌트 체크
        if (auto ThisComponent = dynamic_cast<T*>(this))
            Found.push_back(ThisComponent);

        // 자식 컴포넌트들 검색
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