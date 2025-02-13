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

		// ���� ������Ʈ üũ
		if (auto OwnerComp = Component->GetOwnerComponent())
		{
			// ���� ������Ʈ�� ã�� Ÿ������ Ȯ��
			if (auto TargetComp = dynamic_cast<T*>(OwnerComp))
			{
				return TargetComp;
			}
			// ��������� ���� ������Ʈ Ž��
			return FindOwnerComponentByType<T>(OwnerComp);
		}

		return nullptr;
	}
};

