#pragma once
#include <memory>
#include <vector>
#include <algorithm>
#include <queue>
#include <iostream>

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

    // 컴포넌트 활성화 상태
    void SetActive(bool bNewActive) { bNewActive ? Activate() : DeActivate(); }
    bool IsActive() const { return bIsActive; }

protected:
    virtual void Activate();
    virtual void DeActivate();

protected:
    virtual void PostTreeInitialized()
    {
        SetActive(bIsActive);
    }

    virtual void Tick(float DeltaTime) {}
    // 소유 관계 설정
    void SetOwner(UGameObject* InOwner) { OwnerObject = InOwner; }
    void SetParent(const std::shared_ptr<UActorComponent>& InParent);

public:
    // 컴포넌트 계층 구조 관리
    bool AddChild(const std::shared_ptr<UActorComponent>& Child);
    
    template<typename T>
    bool AddChild(const std::shared_ptr<T>& Child) {
        // T가 UActorComponent에서 파생되었는지 컴파일 타임에 검사
        static_assert(std::is_base_of_v<UActorComponent, T> || std::is_same_v<T, UActorComponent>,
                      "T must inherit from Base");
        // 기존 AddChild 메서드 호출 (타입 캐스팅)
        return AddChild(std::static_pointer_cast<UActorComponent>(Child));
    }

    bool RemoveChild(const std::shared_ptr<UActorComponent>& Child);

    template<typename T>
    bool RemoveChild(const std::shared_ptr<T>& Child) {
        // T가 UActorComponent에서 파생되었는지 컴파일 타임에 검사
        static_assert(std::is_base_of_v<UActorComponent, T> || std::is_same_v<T, UActorComponent>,
                      "T must inherit from Base");
        // 기존 AddChild 메서드 호출 (타입 캐스팅)
        return RemoveChild(std::static_pointer_cast<UActorComponent>(Child));
    }

    void DetachFromParent();

    // 계층 구조 탐색
    UActorComponent* GetRoot() const;
    std::shared_ptr<UActorComponent> GetParent() const { return ParentComponent.lock(); }
    const std::vector<std::shared_ptr<UActorComponent>>& GetChildren() const { return ChildComponents; }

    // 컴포넌트 검색 유틸리티
    template<typename T>
    T* FindComponentByType(const bool SelfInclude = true)
    {
        if (SelfInclude)
        {
            // 현재 컴포넌트 체크
            if (auto ThisComponent = dynamic_cast<T*>(this))
                return ThisComponent;
        }
        
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
        return FindComponentByType<T>(false);
    }

    template<typename T>
    std::vector<std::weak_ptr<T>> FindChildrenByType()
    {
        return FindComponentsByType<T>(false);
    }

    template<typename T>
    std::vector<std::weak_ptr<T>> FindComponentsByType(const bool SelfInclude = true)
    {
        std::vector<std::weak_ptr<T>> Found;

        if (SelfInclude)
        {
            std::shared_ptr<UActorComponent>& SharedThis = shared_from_this();
            Found.push_back(SharedThis);
        }
        
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
    template<typename T>
    std::vector<T*> FindComponentsRaw(const bool SelfInclude = true)
    {
        std::vector<T*> Found;
        
        if (SelfInclude)
        {
            // 현재 컴포넌트 체크
            if (auto ThisComponent = dynamic_cast<const T*>(this))
            {
                Found.push_back(const_cast<T*>(ThisComponent));
            }
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

    template<typename T>
    std::vector<T*> FindChildrenRaw()
    {
        return FindComponentsRaw<T>(false);
    }

private:
    bool bIsActive : 1;

    //생명주기가 종속될것이기에 일반 raw 사용
    UGameObject* OwnerObject = nullptr;
    std::weak_ptr<UActorComponent> ParentComponent;
    std::vector<std::shared_ptr<UActorComponent>> ChildComponents;

    friend class UGameObject;

#pragma region debug
    // ActorComponent.h에 추가
public:
    // 컴포넌트 트리 구조를 출력하는 메서드
    void PrintComponentTree(std::ostream& os = std::cout) const;
    // 컴포넌트의 클래스 이름을 반환하는 가상 메서드
    virtual const char* GetComponentClassName() const { return "UActorComp"; }

private:
    // 내부적으로 트리 출력을 위한 재귀 메서드
    void PrintComponentTreeInternal(std::ostream& os, std::string prefix = "", bool isLast = true) const;

#pragma endregion
};