#include "PrimitiveComponent.h"
#include "Model.h"
#include "Material.h"
#include "Debug.h"
#include "RenderDefines.h"
#include "ResourceManager.h"
#include "TypeCast.h"
#include "RenderDataTexture.h"
#include "define.h"
#include "VertexShader.h"
#include "PixelShader.h"
#include "Texture.h"
#include "Camera.h"

bool UPrimitiveComponent::FillRenderData(const UCamera* Camera, IRenderData* OutRenderData) const 
{
    if (!IsActive())
    {
        LOG("Try FillRender to Invalid UPrimitiveComp");
        return false;
    }

    auto RenderData = Engine::Cast<FRenderDataTexture>(OutRenderData);
    if (!RenderData)
    {
        return false;
    }
    
    //정점 데이터(모델) -  필수 데이터
    auto BufferRsc = GetModel()->GetBufferResource();
    if (!BufferRsc)
    {
        LOG_FUNC_CALL("InValid Model");
        return false;
    }
        
    RenderData->IndexBuffer = BufferRsc->GetIndexBuffer();
    RenderData->IndexCount = BufferRsc->GetIndexCount();
    RenderData->VertexBuffer = BufferRsc->GetVertexBuffer();
    RenderData->VertexCount = BufferRsc->GetVertexCount();
    RenderData->Offset = BufferRsc->GetOffset();
    RenderData->Stride = BufferRsc->GetStride();

    //매터리얼 - 필수 데이터
    auto Material = MaterialHandle.Get<UMaterial>();
    if (!Material)
    {
        LOG_FUNC_CALL("UPrimtiive has Invalid MaterialHandle");
		return false;
    }
    
    //정점 쉐이더 -  필수 데이터
    auto VShader = Material->GetVertexShader();
    if (!VShader)
    {
        LOG_FUNC_CALL("Material Has Invalid VShader");
        return false;
    }

    auto cbVS = VShader->GetAllConstantBufferInfo();
    for (int i = 0; i < cbVS.size(); ++i)
    {
        const auto info = cbVS[i];
        if (info.Name == "MATRIX_BUFFER")
        {
            auto WorldMatrix = GetWorldTransform().GetModelingMatrix();
            WorldMatrix = XMMatrixTranspose(WorldMatrix);

            auto ViewMatrix = Camera->GetViewMatrix();
            ViewMatrix = XMMatrixTranspose(ViewMatrix);

            auto ProjectionMatrix = Camera->GetProjectionMatrix();
            ProjectionMatrix = XMMatrixTranspose(ProjectionMatrix);

            AMatrix192 MatrixData = { WorldMatrix , ViewMatrix, ProjectionMatrix };

            ID3D11Buffer* Buffer = VShader->GetConstantBuffer(i);
            UINT Size = cbVS[i].Size;
            assert(Size == sizeof(MatrixData));
            RenderData->AddVSConstantBuffer(i, Buffer, MatrixData, Size);
        }
        else if (info.Name == "COLOR_BUFFER")
        {
            auto Color = Material->GetColor();
            ID3D11Buffer* Buffer = VShader->GetConstantBuffer(i);
            UINT Size = cbVS[i].Size;
            assert(Size == sizeof(Color));
            RenderData->AddVSConstantBuffer(i, Buffer, Color, Size);
        }
    }

    //텍스처 - 비필수 데이터
    auto Texture = Material->GetTexture();
    if (Texture)
    {
        RenderData->AddTexture(0, Texture->GetShaderResourceView());
    }

    return true;
}

void UPrimitiveComponent::SetModel(const std::shared_ptr<UModel>& InModel)
{
    //TODO
    // 모델데이터도 리소스로 변경후 키값으로 변화 감지.
    Model = InModel;
}

UPrimitiveComponent::UPrimitiveComponent()
{
    //기본 매터리얼
    MaterialHandle = UResourceManager::Get()->LoadResource<UMaterial>(MAT_DEFAULT, true);
}
