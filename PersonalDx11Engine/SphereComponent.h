#pragma once
#include "CollisionComponent.h"
class USphereComponent : public UCollisionComponentBase
{
public:
	// Inherited via UCollisionComponent
	Vector3 GetSupportPoint(const Vector3& Direction) const override;
	Vector3 CalculateInertiaTensor(float Mass) const override;
	void CalculateAABB(Vector3& OutMin, Vector3& OutMax) const override;

	virtual ECollisionShapeType GetType() const override { return ECollisionShapeType::Sphere; }

	virtual const char* GetComponentClassName() const override { return "USphere"; }

protected:
	virtual void RequestDebugRender(const float DeltaTime);

};

