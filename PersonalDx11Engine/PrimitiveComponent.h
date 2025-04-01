#pragma once
#include "SceneComponent.h"
#include "RenderDefines.h"
#include "ResourceHandle.h"

class FResourceHandle;
class UModel;

// 텍스처 렌더링 가능
class UPrimitiveComponent : public USceneComponent
{
public:
	class UModel* GetModel() const { return Model.get(); }
	const Vector4& GetColor() const { return Color; }
	const FResourceHandle& GetTexture() { return TextureHandle; }

	void SetModel(const std::shared_ptr<UModel>& InModel);
	void SetColor(const Vector4& InColor);
	void SetTexture(const FResourceHandle& InHandle);

	virtual const char* GetComponentClassName() const override { return "UPrimitive"; }
private:
	class FResourceHandle TextureHandle;
	std::shared_ptr<class UModel> Model;
	Vector4 Color = Vector4(1, 1, 1, 1); //White
};
