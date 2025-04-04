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
#include "Model.h"

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
    auto Model = ModelHandle.Get<UModel>();
    if (!Model)
    {
        LOG_FUNC_CALL("UPrimtiive has Invalid Model");
        return false;
    }
    RenderData->IndexBuffer = Model->GetIndexBuffer();
    RenderData->IndexCount = Model->GetIndexCount();
    RenderData->VertexBuffer = Model->GetVertexBuffer();
    RenderData->VertexCount = Model->GetVertexCount();
    RenderData->Offset = Model->GetOffset();
    RenderData->Stride = Model->GetStride();

    //매터리얼 - 필수 데이터
    auto Material = MaterialHandle.Get<UMaterial>();
    if (!Material)
    {
        LOG_FUNC_CALL("UPrimtiive has Invalid Material");
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
            //매트릭스 버퍼
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
            //색상 버퍼
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
    else
    {
        //빈 텍스처라는 의미의 nullptr도 전달
        RenderData->AddTexture(0, nullptr);
    }
    
   

    return true;
}

void UPrimitiveComponent::SetModel(const FResourceHandle& InModelHandle)
{
    if (!InModelHandle.IsValid())
        return;

    ModelHandle = InModelHandle;
}

void UPrimitiveComponent::SetMaterial(const FResourceHandle& InMaterialHandle)
{
    if (!InMaterialHandle.IsValid())
        return;

    MaterialHandle = InMaterialHandle;
}

void UPrimitiveComponent::SetColor(const Vector4 InColor)
{
    Color = InColor;
}

UPrimitiveComponent::UPrimitiveComponent()
{
    //기본 매터리얼
    MaterialHandle = UResourceManager::Get()->LoadResource<UMaterial>(MAT_DEFAULT, true);
}
