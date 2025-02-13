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
public:
	static std::shared_ptr<UGameObject> Create()
	{
		return std::shared_ptr<UGameObject>(new UGameObject());
	}

	static std::shared_ptr<UGameObject> Create(const shared_ptr<UModel>& InModel)
	{
		return std::shared_ptr<UGameObject>(new UGameObject(InModel));
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

	const Vector3 GetForwardVector() const;

	__forceinline const FTransform* GetTransform() const { return &Transform; }
	__forceinline FTransform* GetTransform() { return &Transform; }
	Matrix GetWorldMatrix() const  { return Transform.GetModelingMatrix(); }

	void SetModel(const std::shared_ptr<UModel>& InModel) { Model = InModel; }
	UModel* GetModel() const;


	void AddActorComponent(shared_ptr<UActorComponent>& InActorComp);
protected:
	virtual void UpdateComponents(const float DeltaTime);

protected:
	FTransform Transform;
	std::weak_ptr<class UModel> Model;

#pragma region Coord Movement
public:
	void StartMove(const Vector3& InDirection);
	void StopMove();
	void StopMoveImmediately();
	void UpdateMovement(const float DeltaTime);

public:
	//movement test
	bool bIsMoving = false;

	bool IsPhysicsSimulated() const;
	bool IsGravity() const;
   

	void SetGravity(const bool InBool);
	void SetPhysics(const bool InBool);
	void SetFrictionKinetic(const float InValue);
	void SetFrictionStatic(const float InValue);

	Vector3 GetCurrentVelocity() const;
	Vector3 GetCurrentAngularVelocity() const;


	float MaxSpeed = 5.0f; //must be positive

private:
	Vector3 TargetPosition;
#pragma endregion
#pragma region Physics
public:
	void InitializePhysics();
	class URigidBodyComponent* GetRigidBody() { return RigidBody.get(); }
	std::shared_ptr<class URigidBodyComponent>& GetSharedRigidBody() { return RigidBody; }

protected:
	std::shared_ptr<class URigidBodyComponent> RigidBody;
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