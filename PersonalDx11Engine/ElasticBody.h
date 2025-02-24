#pragma once
#include "GameObject.h"
#include "Color.h"
#include "Model.h"

enum class ECollisionShapeType;


class UElasticBody : public UGameObject
{

public:
	enum class EShape
	{
		Box,
		Sphere
	};
	UElasticBody();
	UElasticBody(const Vector4& InColor);
	virtual ~UElasticBody() = default;

public:
	static std::shared_ptr<UElasticBody> Create(const Vector4& InColor = Color::White())
	{
		return std::make_shared<UElasticBody>(InColor);
	}


	virtual void Tick(const float DeltaTime) override;
	virtual void PostInitialized() override;
	virtual void PostInitializedComponents() override;

	void ApplyRandomPhysicsProperties();
	void ApplyRandomTransform();

	virtual void OnCollisionBegin(const struct FCollisionEventData& InCollision) override;
	virtual void OnCollisionStay(const struct FCollisionEventData& InCollision) override;
	virtual void OnCollisionEnd(const struct FCollisionEventData& InCollision) override;

	// Getters  
	const Vector3& GetVelocity() const;
	const Vector3& GetAngularVelocity() const;
	float GetSpeed() const;
	float GetMass() const;
	Vector3 GetRotationalInertia() const;
	float GetRestitution() const;
	float GetFrictionKinetic() const;
	float GetFrictionStatic() const;

	// 물리 속성 설정  
	void SetMass(float InMass);
	void SetMaxSpeed(float InSpeed);
	void SetMaxAngularSpeed(float InSpeed);
	void SetGravityScale(float InScale);
	void SetFrictionKinetic(float InFriction);
	void SetFrictionStatic(float InFriction);
	void SetRestitution(float InRestitution);

private:
	enum class ECollisionShapeType GetCollisionShape(EShape Shape);
protected:
	EShape Shape = EShape::Sphere;
	std::weak_ptr<class URigidBodyComponent> Rigid;
};

