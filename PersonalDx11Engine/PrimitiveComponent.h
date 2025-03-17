#pragma once
#include "SceneCompoent.h"
#include "Model.h"
//렌더링 가능
class UPrimitiveComponent : public USceneComponent
{
public:
	
	UModel* GetModel() const { return Model.get(); }
	const Vector4& GetColor() const { return Color; }

	void SetModel(const std::shared_ptr<UModel>& InModel) { Model = InModel; }
	void SetColor(const Vector4& InColor) { Color = InColor; }

	virtual void Render(class URenderer* Renderer, class UCamera* Camera);

	virtual const char* GetComponentClassName() const override { return "UPrimitive"; }

protected:
	std::shared_ptr<UModel> Model;
	Vector4 Color = Vector4(1, 1, 1, 1); //White

};