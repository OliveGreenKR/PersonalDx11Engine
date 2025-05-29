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

	UTexture2D* GetTexture() const;
	UVertexShader* GetVertexShader() const;
	UPixelShader* GetPixelShader() const;

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
	std::wstring GetPath() const override { return RscPath; }
protected:
	virtual bool LoadImpl(IRenderHardware* RenderHardware, const std::wstring& Path);
	virtual bool LoadAsyncImpl(IRenderHardware* RenderHardware, const std::wstring& Path);
	virtual void ReleaseImpl();

	FResourceHandle TextureHandle = FResourceHandle();
	FResourceHandle VertexShaderHandle;
	FResourceHandle PixelShaderHandle;

private:
	void ReleaseMaterialBase();

private:
	std::wstring RscPath = L"NONE";
	bool bIsLoaded = true; //TODO :  Matrial to File


};

