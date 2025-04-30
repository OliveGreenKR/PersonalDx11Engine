#pragma once
#include "CollisionComponent.h"
class UBoxComponent : public UCollisionComponentBase
{
public:
	// Inherited via UCollisionComponent
	Vector3 GetLocalSupportPoint(const Vector3& WorldDirection) const override;
	Vector3 CalculateInertiaTensor(float Mass) const override;
	void CalculateAABB(Vector3& OutMin, Vector3& OutMax) const override;

	virtual ECollisionShapeType GetType() const override { return ECollisionShapeType::Box; }

	virtual const char* GetComponentClassName() const override { return "UBox"; }

protected:
	virtual void RequestDebugRender(const float DeltaTime);

};

