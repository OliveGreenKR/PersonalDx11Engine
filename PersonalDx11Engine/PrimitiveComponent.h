#pragma once
#include "SceneComponent.h"
#include "ResourceHandle.h"
#include "RenderDataInterface.h"

class FResourceHandle;
class UModel;
class UMaterial;
class UCamera;

// 텍스처 렌더링 가능
class UPrimitiveComponent : public USceneComponent
{
public:
	UPrimitiveComponent();
	virtual ~UPrimitiveComponent() = default;

	virtual bool FillRenderData(const UCamera* Camera, IRenderData* OutRenderData) const;

	void SetModel(const std::shared_ptr<UModel>& InModel);
	void SetMaterial(const FResourceHandle& InMaterialHandle);

	class UModel* GetModel() const { return Model.get(); }
	FResourceHandle GetMaterial() const { return MaterialHandle; }

	virtual const char* GetComponentClassName() const override { return "UPrimitive"; }
private:
	std::shared_ptr<class UModel> Model;
	FResourceHandle MaterialHandle;
};
