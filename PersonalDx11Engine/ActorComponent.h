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
        // T가 Base를 상속받았는지 컴파일 타임에 체크
        static_assert(std::is_base_of_v<UActorComponent, T> || std::is_same_v<T, UActorComponent>,
                      "T must inherit from Base");
        // std::make_shared를 사용하여 객체 생성
        return std::make_shared<T>(std::forward<Args>(args)...);
    }

private:
    // private 복사/이동 생성자와 대입 연산자
    UActorComponent(const UActorComponent&) = delete;
    UActorComponent& operator=(const UActorComponent&) = delete;
    UActorComponent(UActorComponent&&) = delete;
    UActorComponent& operator=(UActorComponent&&) = delete;

public:
    void BraodcastPostTreeInitialized();

    // Tick 전파
    void BroadcastTick(float DeltaTime);

    virtual bool IsEffective() { return  this != nullptr && IsActive(); }

    // 컴포넌트 활성화 상태
    void SetActive(bool bNewActive) { bIsActive = bNewActive; }
    bool IsActive() const { return bIsActive; }

protected:
    // 컴포넌트 트리 구조 완성 후 초기화(오너 및 트리구조 접근 가능)
    virtual void PostTreeInitialized(){}

    virtual void Tick(float DeltaTime) {}
    // 소유 관계 설정
    void SetOwner(UGameObject* InOwner) { OwnerObject = InOwner; }
    void SetParent(const std::shared_ptr<UActorComponent>& InParent);

public:
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
    T* FindComponentByType()
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
    T* FindChildByType()
    {
        // 자식 컴포넌트들 검색
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

        // 자식 컴포넌트들 검색
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


        // 자식 컴포넌트들 검색
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
    // non-const 버전의 raw 포인터 반환 함수도 함께 제공
    template<typename T>
    std::vector<T*> FindComponentsRaw()
    {
        std::vector<T*> Found;

        // 현재 컴포넌트 체크
        if (auto ThisComponent = dynamic_cast<const T*>(this))
        {
            Found.push_back(const_cast<T*>(ThisComponent));
        }

        // 자식 컴포넌트들 검색
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

    // non-const 버전의 raw 포인터 반환 함수도 함께 제공
    template<typename T>
    std::vector<T*> FindChildrenRaw()
    {
        std::vector<T*> Found;

        // 자식 컴포넌트들 검색
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

    //생명주기가 종속될것이기에 일반 raw 사용
    UGameObject* OwnerObject = nullptr;
    std::weak_ptr<UActorComponent> ParentComponent;
    std::vector<std::shared_ptr<UActorComponent>> ChildComponents;

    friend class UGameObject;
};