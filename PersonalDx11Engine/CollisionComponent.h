#pragma once
#include "Math.h"
#include <memory>
#include "Delegate.h"
#include "Transform.h"
#include "CollisionDefines.h"
#include "SceneComponent.h"
#include "CollisionShapeInterface.h"

class UCollisionComponentBase : public USceneComponent, public ICollisionShape
{
#pragma region Constructor and Lifecycle

public:
	UCollisionComponentBase();
	virtual ~UCollisionComponentBase();

#pragma endregion

#pragma region ICollisionShape Implementation

public:
	Vector3 GetScaledHalfExtent() const override;
	Vector3 GetHalfExtent() const override;
	void SetHalfExtent(const Vector3& InHalfExtent) override;

	virtual Vector3 GetWorldSupportPoint(const Vector3& WorldDirection) const = 0;
	virtual Vector3 CalculateInvInertiaTensor(float InvMass) const = 0;
	virtual ECollisionShapeType GetType() const override { return ECollisionShapeType::None; }

#pragma endregion

#pragma region Collision Events

public:
	TDelegate<const FCollisionEvent&> OnCollisionEnter;
	TDelegate<const FCollisionEvent&> OnCollisionStay;
	TDelegate<const FCollisionEvent&> OnCollisionExit;

	void OnCollisionEnterEvent(const FCollisionEvent& CollisionInfo) {
		OnCollisionEnter.Broadcast(CollisionInfo);
	}

	void OnCollisionStayEvent(const FCollisionEvent& CollisionInfo) {
		OnCollisionStay.Broadcast(CollisionInfo);
	}

	void OnCollisionExitEvent(const FCollisionEvent& CollisionInfo) {
		OnCollisionExit.Broadcast(CollisionInfo);
	}

#pragma endregion

#pragma region SceneComponent Overrides

protected:
	virtual void PostInitialized() override;
	virtual void PostTreeInitialized() override;
	virtual void Tick(const float DeltaTime) override;
	virtual void RequestDebugRender(const float DeltaTime) = 0;

public:
	void SetDebugVisualize(const bool InBool) { bIsDebugVisualize = InBool; }
	virtual const char* GetComponentClassName() const override { return "UCollisionionBase"; }

#pragma endregion

#pragma region Data Members

private:
	bool bIsDebugVisualize = false;

#pragma endregion

};