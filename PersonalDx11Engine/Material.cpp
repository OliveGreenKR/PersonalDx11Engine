#include "Material.h"
#include "ResourceManager.h"
#include "define.h"
#include "VertexShader.h"
#include "PixelShader.h"
#include "Texture.h"
#include "Debug.h"


UMaterial::UMaterial()
    : VertexShaderHandle(VS_DEFAULT), PixelShaderHandle(PS_DEFAULT)
{
}

UMaterial::UMaterial(const FResourceHandle& Texture)  
    : TextureHandle(Texture), VertexShaderHandle(VS_DEFAULT), PixelShaderHandle(PS_DEFAULT)
{
}

UMaterial::UMaterial(const FResourceHandle& Texture, const FResourceHandle& VertexShader)
    : TextureHandle(Texture),  VertexShaderHandle(VertexShader) , PixelShaderHandle(PS_DEFAULT)
{
}

UMaterial::UMaterial(const FResourceHandle& Texture, const FResourceHandle& VertexShader, 
                     const FResourceHandle& PixelShader)
    : TextureHandle(Texture), VertexShaderHandle(VertexShader), PixelShaderHandle(PixelShader)
{
}

UMaterial::~UMaterial()
{
    ReleaseMaterialBase();
}

UTexture2D* UMaterial::GetTexture() const
{
    if (!TextureHandle.IsLoaded())
    {
        return nullptr;
    }

    auto RscPtr = TextureHandle.Get<UTexture2D>();
    return RscPtr;
}

UVertexShader* UMaterial::GetVertexShader() const
{
    if (!VertexShaderHandle.IsLoaded())
    {
        return nullptr;
    }

    auto RscPtr = VertexShaderHandle.Get<UVertexShader>();
    return RscPtr;
}

UPixelShader* UMaterial::GetPixelShader() const
{
    if (!PixelShaderHandle.IsLoaded())
    {
        return nullptr;
    }
    auto RscPtr = PixelShaderHandle.Get<UPixelShader>();
    return RscPtr;
}

void UMaterial::SetTexture(const FResourceHandle& InTexture)
{
    if (!InTexture.IsLoaded())
    {
        return;
    }
    TextureHandle = InTexture;
}

void UMaterial::SetVertexShader(const FResourceHandle& InShader)
{
    if (!InShader.IsLoaded())
    {
        return;
    }
    VertexShaderHandle = InShader;
}

void UMaterial::SetPixelShader(const FResourceHandle& InShader)
{
    if (!InShader.IsLoaded())
    {
        return;
    }
    PixelShaderHandle = InShader;
}

bool UMaterial::Load(IRenderHardware* RenderHardware, const std::wstring& Path)
{
    bool result = LoadImpl(RenderHardware, Path);
    if (result)
    {
        bIsLoaded = result;
        RscPath = Path;
    }
    return result;
}

bool UMaterial::LoadAsync(IRenderHardware* RenderHardware, const std::wstring& Path)
{
    bool result = LoadAsyncImpl(RenderHardware, Path);
    if (result)
    {
        bIsLoaded = result;
        RscPath = Path;
    }
    return result;
}

bool UMaterial::LoadImpl(IRenderHardware* RenderHardware, const std::wstring& Path)
{
    bool result = false;

    //TODO Materai Info to File...

    //1. Check Required Resources are Loaded

    //2. Load UnLoaded Required Resources with ResourceManagers.

    if (Path == MAT_TILE)
    {
        TextureHandle = UResourceManager::Get()->LoadResource<UTexture2D>(TEXTURE03);
    }
    else if (Path == MAT_POLE)
    {
        TextureHandle = UResourceManager::Get()->LoadResource<UTexture2D>(TEXTURE02);
    }

    result = true;

    bIsLoaded = result;
    return result;
}

bool UMaterial::LoadAsyncImpl(IRenderHardware* RenderHardware, const std::wstring& Path)
{
    //TOTO Load Async
    return LoadImpl(RenderHardware, Path);
}

void UMaterial::ReleaseImpl()
{
    //MaterialBase Release
}

void UMaterial::ReleaseMaterialBase()
{
}

void UMaterial::Release()
{
    ReleaseMaterialBase();
    ReleaseImpl();
}

size_t UMaterial::GetMemorySize() const
{
    //Not Yet
    return 0;
}
