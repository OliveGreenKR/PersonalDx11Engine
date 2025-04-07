#pragma once
#include "SceneComponent.h"
#include "ResourceHandle.h"
#include "RenderDataInterface.h"
#include "Math.h"

class FResourceHandle;
class UModel;
class UMaterial;
class UCamera;

// 인스턴스별 렌더링 데이터
class UPrimitiveComponent : public USceneComponent
{
public:
	UPrimitiveComponent();
	virtual ~UPrimitiveComponent() = default;

	virtual bool FillRenderData(const UCamera* Camera, IRenderData* OutRenderData) const;

	void SetModel(const FResourceHandle& InModelHandle);
	void SetMaterial(const FResourceHandle& InMaterialHandle);
	void SetColor(const Vector4 InColor);

	FResourceHandle GetModel() const { return ModelHandle; }
	FResourceHandle GetMaterial() const { return MaterialHandle; }
	Vector4 GetColor() const { return Color; }

	virtual const char* GetComponentClassName() const override { return "UPrimitive"; }
private:
	FResourceHandle ModelHandle = FResourceHandle();
	FResourceHandle MaterialHandle = FResourceHandle();
	Vector4 Color = Vector4(1, 1, 1, 1);
};
