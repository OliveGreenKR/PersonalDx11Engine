#include "PrimitiveComponent.h"
#include "ModelBufferManager.h"
#include "RenderDataTexture.h"
#include "ResourceManager.h"
#include "Model.h"
#include "Texture.h"
#include "VertexShader.h"
#include "Camera.h"
#include "Debug.h"

void UPrimitiveComponent::SetModel(const std::shared_ptr<UModel>& InModel)
{
    //TODO
    // 모델데이터도 리소스로 변경후 키값으로 변화 감지.
    Model = InModel;
    bRenderDataDirty = true;
}

void UPrimitiveComponent::SetColor(const Vector4& InColor) 
{
    if (Color == InColor)
    {
        return;
    }
    Color = InColor;
    bRenderDataDirty = true;
}

void UPrimitiveComponent::SetTexture(const FResourceHandle& InHandle)
{
    if (!InHandle.IsValid() || TextureHandle == InHandle )
    {
        return; 
    }
    TextureHandle = InHandle;
    bRenderDataDirty = true;
}

std::weak_ptr<IRenderData> UPrimitiveComponent::GetRenderData(const FResourceHandle& VertexShaderHandle)
{
    if (!Model || !VertexShaderHandle.IsValid())
        return std::weak_ptr<IRenderData>();

    if (!bRenderDataDirty  && RenderDataCache)
    {
        return RenderDataCache;
    }

    auto BufferRsc = GetModel()->GetBufferResource();
    auto RenderData = std::make_shared<FTextureRenderData>();

    RenderData->IndexBuffer = BufferRsc->GetIndexBuffer();
    RenderData->IndexCount = BufferRsc->GetIndexCount();
    RenderData->VertexBuffer = BufferRsc->GetVertexBuffer();

    RenderData->VertexCount = BufferRsc->GetVertexCount();
    RenderData->Offset = BufferRsc->GetOffset();
    RenderData->Stride = BufferRsc->GetStride();

    auto Texture = TextureHandle.Get<UTexture2D>();
    if (Texture)
    {
        RenderData->AddTexture(0, Texture->GetShaderResourceView());
    }

    //Shader Constant Buffer
    auto Shader = VertexShaderHandle.Get<UVertexShader>();
    if (!Shader)
    {
        LOG("InValid Shader In PrimitiveComponent::GetRenderData");
        return;
    }
        
    auto cbVS = Shader->GetAllConstantBufferInfo();

    for (int i = 0; i < cbVS.size() ; ++i)
    {
        const auto info = cbVS[i];
        if (info.Name == "MATRIX_WORLD")
        {
            CachedWorldMatrixTrans = GetWorldTransform().GetModelingMatrix();
            CachedWorldMatrixTrans = XMMatrixTranspose(CachedWorldMatrixTrans);
            ID3D11Buffer* Buffer = Shader->GetConstantBuffer(i);
            UINT Size = cbVS[i].Size;
            assert(Size == sizeof(CachedWorldMatrixTrans));
            RenderData->AddVSConstantBuffer(i, Buffer, &CachedWorldMatrixTrans, Size);
        }
        else if (info.Name == "COLOR_BUFFER")
        {
            ID3D11Buffer* Buffer = Shader->GetConstantBuffer(i);
            UINT Size = cbVS[i].Size;
            assert(Size == sizeof(Color));
            RenderData->AddVSConstantBuffer(i, Buffer, &Color, Size);
        }
    }

    bRenderDataDirty = false;
    RenderDataCache = std::move(RenderData);
    return RenderDataCache;
}
