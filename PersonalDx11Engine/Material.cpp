#include "Material.h"
#include "ResourceManager.h"
#include "define.h"
#include "VertexShader.h"
#include "PixelShader.h"
#include "Texture.h"



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

UMaterial::UMaterial(const FResourceHandle& Texture, const FResourceHandle& VertexShader, const FResourceHandle& PixelShader, const ERenderStateType InRenderStateType)
    : TextureHandle(Texture), VertexShaderHandle(VertexShader), PixelShaderHandle(PixelShader), RenderState(InRenderStateType)
{
}

Vector4 UMaterial::GetColor() const
{
    return Color;
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

ERenderStateType UMaterial::GetRenderState() const
{
    return RenderState;
}

void UMaterial::SetColor(const Vector4& InColor)
{
    Color = InColor;
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

void UMaterial::SetRenderState(const ERenderStateType InRenderStateType)
{
    if (InRenderStateType == ERenderStateType::None)
    {
        return;
    }
    RenderState = InRenderStateType;
}
