#pragma once
#include "Transform.h"
#include <memory>
#include "Color.h"
#include "Delegate.h"
#include "ActorComponent.h"

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

	template<typename T>
	static std::shared_ptr<T> Create(const shared_ptr<UModel>& InModel)
	{
		static_assert(std::is_base_of_v<UGameObject, T> || std::is_same_v<UGameObject, T>,
					  "T must be derived of UGameObject");

		return std::make_shared<ConstructorAccess<T>>(InModel);
	}

protected:
	UGameObject();
	UGameObject(const shared_ptr<UModel>& InModel);
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
	void SetRotationQuaternion(const Quaternion& InQuaternion);
	void SetScale(const Vector3& InScale);

	void AddPosition(const Vector3& InDelta);
	void AddRotationEuler(const Vector3& InEulerDelta);
	void AddRotationQuaternion(const Quaternion& InQuaternionDelta);

	const Vector3 GetNormalizedForwardVector() const;

	__forceinline const FTransform* GetTransform() const { return &Transform; }
	__forceinline FTransform* GetTransform() { return &Transform; }
	Matrix GetWorldMatrix() const  { return Transform.GetModelingMatrix(); }

	void SetModel(const std::shared_ptr<UModel>& InModel) { Model = InModel; }
	UModel* GetModel() const;

protected:
	virtual void UpdateComponents(const float DeltaTime);

protected:
	FTransform Transform;
	std::weak_ptr<class UModel> Model;
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

	//void AddActorComponent(shared_ptr<UActorComponent>& InActorComp);
	void AddActorComponent(const shared_ptr<UActorComponent>& InActorComp);
	UActorComponent* GetRootActorComp() { return RootActorComp.get(); }

protected:
	std::shared_ptr<UActorComponent> RootActorComp;
#pragma endregion

#pragma region Debug
public:
	bool bDebug = false;

	void SetDebugColor(const Vector4& InColor) { DebugColor = InColor; }
	Vector4 GetDebugColor() const  { return DebugColor; }

protected:
	Vector4 DebugColor = Color::White();
#pragma endregion
};