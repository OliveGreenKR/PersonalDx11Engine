#pragma once
#include <memory>
#include <vector>
#include <algorithm>
#include <queue>
#include <iostream>
#include "TypeCast.h"
#include "Object.h"

class UGameObject;

class UActorComponent : public std::enable_shared_from_this<UActorComponent>, public UObject
{
public:
	// 토큰 클래스
	class OwnerToken {
		friend class UGameObject;  // UGameObject만 토큰 생성 가능
	private:
		OwnerToken() = default;
	};

public:
    UActorComponent() : bIsActive(true) {}
    virtual ~UActorComponent();

	// 토큰이 있어야만 호출 가능한 메서드(외부 설정을 위함)
	void RequestSetOwner(UGameObject* InOwner, const OwnerToken&) { SetOwner(InOwner); }

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
	//Called When GameObject::PostInitialzed
	void BroadcastPostInitialized();
	//Called When GameObject::PostInitialzedComponents
    void BroadcastPostTreeInitialized();

    // Tick 전파
    void BroadcastTick(float DeltaTime);

    // 컴포넌트 활성화 상태
	void SetActive(bool bNewActive);
    bool IsActive() const { return bIsActive; }

protected:
    virtual void Activate();
    virtual void DeActivate();

protected:
	virtual void PostInitialized();
	virtual void PostTreeInitialized();

    virtual void Tick(float DeltaTime) {}

protected:

    // 소유자 설정
    void SetOwner(UGameObject* InOwner) { OwnerObject = InOwner; }
	// 계층 구조 설정 내부 함수
    void SetParent(const std::shared_ptr<UActorComponent>& InParent);
	// 부모 변경 시 호출되는 가상 함수
	virtual void OnParentChanged(const std::shared_ptr<UActorComponent>& NewParent){}
private:
	// 계층 구조 내부 헬퍼
	void SetParentInternal(const std::shared_ptr<UActorComponent>& InParent, const bool ShouldCallEvent );

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

#pragma region FindComponent
public:
	// 계층 구조 탐색
	UActorComponent* GetRoot() const;
	std::shared_ptr<UActorComponent> GetParent() const { return ParentComponent.lock(); }
	const std::vector<std::shared_ptr<UActorComponent>>& GetChildren() const { return ChildComponents; }

	template<typename T>
	std::weak_ptr<T> FindComponentByType(const bool SelfInclude = true)
	{
		// 현재 컴포넌트 검사 (옵션에 따라)
		if (SelfInclude)
		{
			// 현재 컴포넌트가 요청된 타입인지 확인
			if (dynamic_cast<T*>(this))
			{
				// 안전하게 캐스팅하고 weak_ptr로 변환
				return Engine::Cast<T>(shared_from_this());
			}
		}

		// 자식 컴포넌트들 검색
		for (const auto& Child : ChildComponents)
		{
			if (Child)
			{
				auto FoundComponent = Child->FindComponentByType<T>();
				if (!FoundComponent.expired())
					return FoundComponent;
			}
		}

		return std::weak_ptr<T>();
	}

	template<typename T>
	std::weak_ptr<T> FindChildByType() 
	{
		return FindComponentByType<T>(false);
	}

	template<typename T>
	std::vector<std::weak_ptr<T>> FindComponentsByType(const bool SelfInclude = true)
	{
		std::vector<std::weak_ptr<T>> Found;

		if (SelfInclude)
		{
			// 현재 컴포넌트 검사
			if (dynamic_cast<T*>(this))
			{
				// 안전하게 캐스팅하고 weak_ptr로 변환
				Found.push_back(Engine::Cast<T>(shared_from_this()));
			}
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

	template<typename T>
	std::vector<std::weak_ptr<T>> FindChildrenByType() 
	{
		return FindComponentsByType<T>(false);
	}

	template<typename T>
	T* FindComponentRaw(const bool SelfInclude = true)
	{
		if (SelfInclude)
		{
			// 현재 컴포넌트 검사
			if (auto ThisComponent = dynamic_cast<T*>(this))
			{
				return ThisComponent;
			}
		}

		// 자식 컴포넌트들 검색
		for (const auto& Child : ChildComponents)
		{
			if (Child)
			{
				if (auto FoundComponent = Child->FindComponentRaw<T>())
					return FoundComponent;
			}
		}

		return nullptr;
	}

	template<typename T>
	std::vector<T*> FindComponentsRaw(const bool SelfInclude = true)
	{
		std::vector<T*> Found;

		if (SelfInclude)
		{
			// 현재 컴포넌트 체크
			if (auto ThisComponent = dynamic_cast<T*>(this))
			{
				Found.push_back(ThisComponent);
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
		// 자기 자신을 제외하고 자식들만 검색
		return FindComponentsRaw<T>(false);
	}

	template<typename T>
	std::weak_ptr<T> FindComponentByType(const bool SelfInclude = true) const
	{
		// 현재 컴포넌트 검사 (옵션에 따라)
		if (SelfInclude)
		{
			// 현재 컴포넌트가 요청된 타입인지 확인
			if (dynamic_cast<const T*>(this))
			{
				// const 객체에서는 shared_from_this()를 직접 호출할 수 없으므로
				// const_cast를 사용하여 일시적으로 const를 제거
				auto nonConstThis = const_cast<UActorComponent*>(this);
				return Engine::Cast<T>(nonConstThis->shared_from_this());
			}
		}

		// 자식 컴포넌트들 검색
		for (const auto& Child : ChildComponents)
		{
			if (Child)
			{
				auto FoundComponent = Child->FindComponentByType<T>();
				if (!FoundComponent.expired())
					return FoundComponent;
			}
		}

		return std::weak_ptr<T>();
	}

	template<typename T>
	std::weak_ptr<T> FindChildByType() const
	{
		// 자기 자신을 제외하고 자식들만 검색
		return FindComponentByType<T>(false);
	}

	template<typename T>
	std::vector<std::weak_ptr<T>> FindComponentsByType(const bool SelfInclude = true) const
	{
		std::vector<std::weak_ptr<T>> Found;

		if (SelfInclude)
		{
			// 현재 컴포넌트 검사
			if (dynamic_cast<const T*>(this))
			{
				// const 객체에서는 shared_from_this()를 직접 호출할 수 없으므로
				// const_cast를 사용하여 일시적으로 const를 제거
				auto nonConstThis = const_cast<UActorComponent*>(this);
				Found.push_back(Engine::Cast<T>(nonConstThis->shared_from_this()));
			}
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

	template<typename T>
	std::vector<std::weak_ptr<T>> FindChildrenByType() const
	{
		// 자기 자신을 제외하고 자식들만 검색
		return FindComponentsByType<T>(false);
	}

	template<typename T>
	T* FindComponentRaw(const bool SelfInclude = true) const
	{
		if (SelfInclude)
		{
			// 현재 컴포넌트 검사
			if (auto ThisComponent = dynamic_cast<const T*>(this))
			{
				return const_cast<T*>(ThisComponent);
			}
		}

		// 자식 컴포넌트들 검색
		for (const auto& Child : ChildComponents)
		{
			if (Child)
			{
				if (auto FoundComponent = Child->FindComponentRaw<T>())
					return FoundComponent;
			}
		}

		return nullptr;
	}

	template<typename T>
	std::vector<T*> FindComponentsRaw(const bool SelfInclude = true) const
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
	std::vector<T*> FindChildrenRaw() const
	{
		// 자기 자신을 제외하고 자식들만 검색
		return FindComponentsRaw<T>(false);
	}
#pragma endregion
private:
    bool bIsActive : 1;

    //생명주기가 종속될것이기에 일반 raw 사용
    UGameObject* OwnerObject = nullptr;
    std::weak_ptr<UActorComponent> ParentComponent;
    std::vector<std::shared_ptr<UActorComponent>> ChildComponents;

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