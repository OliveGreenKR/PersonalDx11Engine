#pragma once
#include "SceneComponent.h"
#include "RenderDefines.h"
#include "ResourceHandle.h"
#include "MaterialInterface.h"

class FResourceHandle;
class UModel;

// 텍스처 렌더링 가능
class UPrimitiveComponent : public USceneComponent
{
public:
	virtual void PostInitialized() override;

	class UModel* GetModel() const { return Model.get(); }
	IMaterial* GetMaterial() { return Material.get();}

	virtual const char* GetComponentClassName() const override { return "UPrimitive"; }
private:
	std::shared_ptr<class UModel> Model;
	std::shared_ptr<IMaterial> Material;
};
