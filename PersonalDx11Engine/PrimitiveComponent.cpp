#include "PrimitiveComponent.h"
#include "ModelBufferManager.h"
#include "RenderDataTexture.h"
#include "ResourceManager.h"
#include "Model.h"
#include "Texture.h"
#include "Camera.h"

inline void UPrimitiveComponent::SetModel(const std::shared_ptr<UModel>& InModel)
{
    //TODO
    // 모델데이터도 리소스로 변경후 키값으로 변화 감지.
    Model = InModel;
    bRenderDataDirty = true;
}

inline void UPrimitiveComponent::SetColor(const Vector4& InColor) 
{
    Color = InColor;
    bRenderDataDirty = true;
}

std::weak_ptr<IRenderData> UPrimitiveComponent::GetRenderData()
{
    if (!Model || !TextureHandle)
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

    auto Texture = TextureHandle->Get<UTexture2D>();
 
    if (TextureHandle)
    {
        RenderData->AddTexture(0, Texture->GetShaderResourceView());
    }

    //shader resource - texture, sampler
    //RenderData->AddSampler(0, Renderer->GetDefaultSamplerState());
    

    XMMATRIX world = GetWorldTransform().GetModelingMatrix();
    XMMATRIX view = Camera->GetViewMatrix();
    XMMATRIX proj = Camera->GetProjectionMatrix();

    world = XMMatrixTranspose(world);
    view = XMMatrixTranspose(view);
    proj = XMMatrixTranspose(proj);

    std::memcpy(MatrixBufferData, &world, sizeof(XMMATRIX));
    std::memcpy(static_cast<char*>(MatrixBufferData) + sizeof(XMMATRIX), &view, sizeof(XMMATRIX));
    std::memcpy(static_cast<char*>(MatrixBufferData) + sizeof(XMMATRIX) * 2, &proj, sizeof(XMMATRIX));

    Vector4 color(1, 1, 1, 1);
    std::memcpy(ColorBufferData, &color, sizeof(Vector4));

    auto cbVS = Shader->GetAllConstantBufferInfo();
    auto MatrixBuffer = cbVS[0].Buffer;
    auto ColorBuffer = cbVS[1].Buffer;

    RenderData->AddVSConstantBuffer(0, MatrixBuffer, MatrixBufferData, cbVS[0].Size);
    RenderData->AddVSConstantBuffer(1, ColorBuffer, ColorBufferData, cbVS[1].Size);



    return std::weak_ptr<IRenderData>();
}
