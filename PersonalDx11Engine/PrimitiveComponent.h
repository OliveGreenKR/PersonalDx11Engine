#pragma once
#include "SceneComponent.h"
#include "RenderDefines.h"

class FResourceHandle;

// 텍스처 렌더링 가능
class UPrimitiveComponent : public USceneComponent
{
public:
	
	UModel* GetModel() const { return Model.get(); }
	const Vector4& GetColor() const { return Color; }
	const FResourceHandle& GetTexture() { return TextureHandle; }

	void SetModel(const std::shared_ptr<UModel>& InModel);
	void SetColor(const Vector4& InColor);
	void SetTexture(const FResourceHandle& InHandle);

	virtual const char* GetComponentClassName() const override { return "UPrimitive"; }

	virtual std::weak_ptr<IRenderData> GetRenderData(const FResourceHandle& VertexShaderHandle);

private:
	class FResourceHandle TextureHandle;
	std::shared_ptr<class UModel> Model;
	Vector4 Color = Vector4(1, 1, 1, 1); //White
	XMMATRIX CachedWorldMatrixTrans = XMMatrixIdentity();

protected:
	std::shared_ptr<IRenderData> RenderDataCache;
	bool bRenderDataDirty = true;
};
