#pragma once
#include <memory>

class UActorComponent : public std::enable_shared_from_this<UActorComponent>
{
public:
	virtual void PostInitialized() {}
	virtual void Tick(const float DeltaTime) {}

	virtual class UGameObject* GetOwner() const = 0;
	virtual const UActorComponent* GetOwnerComponent() const = 0;

	template<typename T>
	static T* FindOwnerComponentByType(const UActorComponent* Component)
	{
		if (!Component)
			return nullptr;

		// 상위 컴포넌트 체크
		if (auto OwnerComp = Component->GetOwnerComponent())
		{
			// 현재 컴포넌트가 찾는 타입인지 확인
			if (auto TargetComp = dynamic_cast<T*>(OwnerComp))
			{
				return TargetComp;
			}
			// 재귀적으로 상위 컴포넌트 탐색
			return FindOwnerComponentByType<T>(OwnerComp);
		}

		return nullptr;
	}
};

