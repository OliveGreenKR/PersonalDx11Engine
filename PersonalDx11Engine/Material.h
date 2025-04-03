#pragma once
#include "MaterialInterface.h"
#include "ResourceHandle.h"
class UMaterial : public IMaterial
{
public:
	explicit UMaterial();
	explicit UMaterial(const FResourceHandle& Texture);
	explicit UMaterial(const FResourceHandle& Texture,
					   const FResourceHandle& VertexShader);
	explicit UMaterial(const FResourceHandle& Texture, 
					   const FResourceHandle& VertexShader,
					   const FResourceHandle& PixelShader,
					   const ERenderStateType InType = ERenderStateType::Solid);

	virtual ~UMaterial() = default;

	// Inherited via IMaterial
	Vector4 GetColor() const override;
	UTexture2D* GetTexture() const override;
	UVertexShader* GetVertexShader() const override;
	UPixelShader* GetPixelShader() const override;
	ERenderStateType GetRenderState() const override;

	void SetColor(const Vector4& InColor);
	void SetTexture(const FResourceHandle& InTexture);
	void SetVertexShader(const FResourceHandle& InShader);
	void SetPixelShader(const FResourceHandle& InShader);
	void SetRenderState(const ERenderStateType InRenderStateType);

protected:
	FResourceHandle TextureHandle = FResourceHandle();
	FResourceHandle VertexShaderHandle;
	FResourceHandle PixelShaderHandle;
	Vector4 Color = Vector4(1, 1, 1, 1);
	ERenderStateType RenderState = ERenderStateType::Solid;
};

