#pragma once
#include "Transform.h"
#include <memory>
#include "Color.h"
#include "Delegate.h"
#include "ActorComponent.h"
#include "SceneComponent.h"

using namespace std;

class UModel;

class UGameObject : public std::enable_shared_from_this<UGameObject>
{
private:
	template<typename T>
	struct ConstructorAccess : public T {
		ConstructorAccess() : T() {}

		template<typename... Args>
		ConstructorAccess(Args&&... args) : T(std::forward<Args>(args)...) {}
	};
public:

	// 객체 생성을 위한 템플릿 팩토리 메소드
	template<typename T>
	static std::shared_ptr<T> Create()
	{
		static_assert(std::is_base_of_v<UGameObject, T> || std::is_same_v<UGameObject, T>,
					  "T must be derived of UGameObject");

		// make_shared 로 효율적인 한번의 할당만
		return std::make_shared<ConstructorAccess<T>>();
	}

	template<typename T, typename...Args>
	static std::shared_ptr<T> Create(Args&&...args)
	{
		static_assert(std::is_base_of_v<UGameObject, T> || std::is_same_v<UGameObject, T>,
					  "T must be derived of UGameObject");

		return std::make_shared<ConstructorAccess<T>>(std::forward<Args>(args)...);
	}

protected:
	UGameObject();
public:
	virtual ~UGameObject() = default;

public:
	virtual void PostInitialized();
	virtual void PostInitializedComponents();
	virtual void Tick(const float DeltaTime) ;

public:
	void SetPosition(const Vector3& InPosition);
	//{ Pitch, Yaw, Roll } in Degree
	void SetRotationEuler(const Vector3& InEulerAngles);
	void SetRotation(const Quaternion& InQuaternion);
	void SetScale(const Vector3& InScale);

	void AddPosition(const Vector3& InDelta);
	void AddRotationEuler(const Vector3& InEulerDelta);
	void AddRotationQuaternion(const Quaternion& InQuaternionDelta);

	const Vector3 GetNormalizedForwardVector() const;

	__forceinline const FTransform& GetTransform() const { return RootComponent->GetWorldTransform(); }
	Matrix GetWorldMatrix() const  { return RootComponent->GetWorldTransform().GetModelingMatrix(); }

protected:
	virtual void UpdateComponents(const float DeltaTime);

protected:
	//FTransform Transform;
	//std::weak_ptr<class UModel> Model;

#pragma region Activation
public:
		// 활성화/비활성화
	void SetActive(const bool bActive) { bActive ? Activate() : DeActivate(); }
	bool IsActive() const { return bIsActive; }

	virtual void Activate();
	virtual void DeActivate();

private:
	bool bIsActive = true;

#pragma region EventTriggered
	virtual void OnCollisionBegin(const struct FCollisionEventData& InCollision);
	virtual void OnCollisionStay(const struct FCollisionEventData& InCollision) {}
	virtual void OnCollisionEnd(const struct FCollisionEventData& InCollision);
#pragma endregion
#pragma region Coord Movement
public:
	void StartMove(const Vector3& InDirection);
	void StopMove();
	void StopMoveImmediately();
	void UpdateMovement(const float DeltaTime);
public:
	//movement test
	bool bIsMoving = false;

private:
	float MaxSpeed = 5.0f;
	Vector3 TargetPosition;
#pragma endregion
#pragma region Physics
public:
	void ApplyForce(const Vector3&& InForce);
	void ApplyImpulse(const Vector3&& InImpulse);

	Vector3 GetCurrentVelocity() const;
	float GetMass() const;

	bool IsGravity() const;
	bool IsPhysicsSimulated() const;

	void SetGravity(const bool InBool);
	void SetPhysics(const bool InBool);
#pragma endregion
#pragma region ActorComp
public:
	// 컴포넌트  생성 및  추가 (루트 컴포넌트의 자식으로)
	template<typename T, typename... Args>
	std::shared_ptr<T> AddComponent(Args&&... args) {
		auto component = UActorComponent::Create<T>(std::forward<Args>(args)...);
		if (RootComponent) {
			RootComponent->AddChild(component);
		}
		else {
			//최초 루트 설정
			RootComponent = component;
			RootComponent->RequestSetOwner(this, UActorComponent::OwnerToken());
		}
		return component;
	}

	// 타입으로 컴포넌트 찾기
	template<typename T>
	T* GetComponentByType() {
		if (!RootComponent) return nullptr;
		return RootComponent->FindComponentRaw<T>();
	}

	// 특정 타입의 모든 컴포넌트 찾기
	template<typename T>
	std::vector<T*> GetComponentsByType() {
		if (!RootComponent) return {};
		return RootComponent->FindChildrenRaw<T>();
	}
	std::shared_ptr<USceneComponent>& GetRootComp() { return RootComponent; }

protected:
	template <typename T>
	std::weak_ptr<T> CreateRootComponent()
	{
		if (std::is_base_of_v<USceneComponent,T> || std::is_same_v<USceneComponent, T>)
		{
			if (RootComponent)
			{
				RootComponent = nullptr;
			}
			auto NewScene = USceneComponent::Create<T>();
			RootComponent = NewScene;
			RootComponent->RequestSetOwner(this, UActorComponent::OwnerToken());
			return NewScene;
		}
		return std::weak_ptr<T>();
	}

private:
	std::shared_ptr<USceneComponent> RootComponent;
#pragma endregion

};