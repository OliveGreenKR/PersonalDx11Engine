#pragma once
#include "ResourceInterface.h"
#include "ResourceHandle.h"
#include "Math.h"

class UTexture2D;
class UVertexShader;
class UPixelShader;

class UMaterial : public IResource
{
public:
	explicit UMaterial();
	explicit UMaterial(const FResourceHandle& Texture);
	explicit UMaterial(const FResourceHandle& Texture,
					   const FResourceHandle& VertexShader);
	explicit UMaterial(const FResourceHandle& Texture, 
					   const FResourceHandle& VertexShader,
					   const FResourceHandle& PixelShader);

	virtual ~UMaterial();

	Vector4 GetColor() const;
	UTexture2D* GetTexture() const;
	UVertexShader* GetVertexShader() const;
	UPixelShader* GetPixelShader() const;

	void SetColor(const Vector4& InColor);
	void SetTexture(const FResourceHandle& InTexture);
	void SetVertexShader(const FResourceHandle& InShader);
	void SetPixelShader(const FResourceHandle& InShader);

	// Inherited via IResource
	bool Load(IRenderHardware* RenderHardware, const std::wstring& Path) override;
	bool LoadAsync(IRenderHardware* RenderHardware, const std::wstring& Path) override;
	bool IsLoaded() const override { return bIsLoaded; }
	void Release() override;

	size_t GetMemorySize() const override;
	EResourceType GetType() const override { return EResourceType::Material; }

protected:
	FResourceHandle TextureHandle = FResourceHandle();
	FResourceHandle VertexShaderHandle;
	FResourceHandle PixelShaderHandle;
	Vector4 Color = Vector4(1, 1, 1, 1);

private:
	bool bIsLoaded = true; //TODO :  Matrial to File


};

