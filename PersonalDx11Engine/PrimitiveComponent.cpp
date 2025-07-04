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
#include "FramePoolManager.h"

bool UPrimitiveComponent::FillRenderData(const UCamera* Camera, IRenderData* OutRenderData) const 
{
    if (!IsActive())
    {
        return false;
    }

    auto RenderData = OutRenderData;
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

    RenderData->SetIndexBuffer(Model->GetIndexBuffer());
    RenderData->SetIndexCount( Model->GetIndexCount());
    RenderData->SetVertexBuffer( Model->GetVertexBuffer());
    RenderData->SetVertexCount( Model->GetVertexCount());
    RenderData->SetOffset( Model->GetOffset());
    RenderData->SetStride( Model->GetStride());

    //매터리얼 - 필수 데이터
    auto Material = MaterialHandle.Get<UMaterial>();
    if (!Material)
    {
        LOG_ERROR("UPrimtiive has Invalid Material");
		return false;
    }
    
    //정점 쉐이더 -  필수 데이터
    auto VShader = Material->GetVertexShader();
    if (!VShader)
    {
        LOG_ERROR("Material Has Invalid VShader");
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
            auto WorldInverse = XMMatrixInverse(nullptr, WorldMatrix);
            auto ViewMatrix = Camera->GetViewMatrix();
            auto ProjectionMatrix = Camera->GetProjectionMatrix();



            WorldMatrix = XMMatrixTranspose(WorldMatrix);
            WorldInverse = XMMatrixTranspose(WorldInverse);
            ViewMatrix = XMMatrixTranspose(ViewMatrix);
            ProjectionMatrix = XMMatrixTranspose(ProjectionMatrix);

            AMatrix256 MatrixData = { WorldMatrix , ViewMatrix, ProjectionMatrix, WorldInverse };

            ID3D11Buffer* Buffer = VShader->GetConstantBuffer(i);
            UINT Size = cbVS[i].Size;
            assert(Size == sizeof(MatrixData));

            auto data = UFramePoolManager::Get()->AllocateVoid(Size);
            std::memcpy(data, &MatrixData, Size);

            RenderData->AddVSConstantBuffer(i, Buffer, data, Size);
        }
        else if (info.Name == "COLOR_BUFFER")
        {
            //색상 버퍼
            ID3D11Buffer* Buffer = VShader->GetConstantBuffer(i);
            UINT Size = cbVS[i].Size;
            assert(Size == sizeof(Color));


            auto data = UFramePoolManager::Get()->AllocateVoid(Size);
            std::memcpy(data, &Color, Size);

            RenderData->AddVSConstantBuffer(i, Buffer, data, Size);
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
