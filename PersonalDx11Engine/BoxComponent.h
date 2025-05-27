#pragma once
#include "CollisionComponent.h"
class UBoxComponent : public UCollisionComponentBase
{
public:
	// Inherited via UCollisionComponent
	Vector3 GetWorldSupportPoint(const Vector3& WorldDirection) const override;
	Vector3 CalculateInvInertiaTensor(float InvMass) const override;
	void CalculateAABB(Vector3& OutMin, Vector3& OutMax) const override;

	virtual ECollisionShapeType GetType() const override { return ECollisionShapeType::Box; }

	virtual const char* GetComponentClassName() const override { return "UBox"; }

protected:
	virtual void RequestDebugRender(const float DeltaTime);

};

