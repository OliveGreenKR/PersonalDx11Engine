#pragma once
#include "GameObject.h"
#include "Color.h"

enum class ECollisionShapeType;


class UElasticBody : public UGameObject
{

public:
	enum class EShape
	{
		Cube,
		Sphere
	};
	UElasticBody(const EShape& InShape, const Vector4& InColor);
	virtual ~UElasticBody() = default;

public:
	static std::shared_ptr<UElasticBody> Create(const EShape& InShape = EShape::Sphere, const Vector4& InColor = Color::White())
	{
		return std::make_shared<UElasticBody>(InShape, InColor);
	}


	virtual void Tick(const float DeltaTime) override;
	virtual void PostInitialized() override;
	virtual void PostInitializedComponents() override;

	virtual void OnCollisionBegin(const struct FCollisionEventData& InCollision) override;
	virtual void OnCollisionStay(const struct FCollisionEventData& InCollision) override;
	virtual void OnCollisionEnd(const struct FCollisionEventData& InCollision) override;


protected:
	EShape Shape = EShape::Sphere;
};

