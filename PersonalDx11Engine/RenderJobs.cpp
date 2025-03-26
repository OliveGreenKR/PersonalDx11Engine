#include "RenderJobs.h"
#include "ShaderInterface.h"
#include "RenderContext.h"
#include "Debug.h"

void FTextureRenderJob::Execute(FRenderContext* Context)
{
     // 1. 버퍼 바인딩
    if (VertexBuffer)
    {
        Context->BindVertexBuffer(VertexBuffer, Stride, Offset);
    }
    if (IndexBuffer)
    {
        Context->BindIndexBuffer(IndexBuffer);
    }
   
    // 3. 상수 버퍼 바인딩 
    for (const auto& CB : VSConstantBuffers)
    {
        Context->BindConstantBuffer(CB.Slot, CB.Buffer, CB.Data, CB.DataSize, true);
    }
    for (const auto& CB : PSConstantBuffers)
    {
        Context->BindConstantBuffer(CB.Slot, CB.Buffer, CB.Data, CB.DataSize, false);
    }
        
    // 4. 텍스처 및 쉐이더 리소스
    for (const auto& Tex : Textures)
    {
        Context->BindShaderResource(Tex.Slot, Tex.SRV);
    }
    // 5. 샘플러
    for (const auto& Samp : Samplers)
    {
        Context->BindSamplerState(Samp.Slot, Samp.Sampler);
    }

    // 6. 드로우 콜 실행
    if (IndexCount > 0)
    {
        Context->DrawIndexed(IndexCount, StartIndex, BaseVertex);
    } 
    else if (VertexCount > 0)
    {
        Context->Draw(VertexCount, StartVertex);
    }
        
}


void FTextureRenderJob::AddVSConstantBuffer(uint32_t Slot, ID3D11Buffer* Buffer, void* Data, size_t DataSize)
{
    if (!Buffer || !Data)
    {
        LOG("Wrong ConstantBuffer");
        return;
    }
	VSConstantBuffers.push_back({ Slot, Buffer, Data, DataSize });
}

void FTextureRenderJob::AddPSConstantBuffer(uint32_t Slot, ID3D11Buffer* Buffer, void* Data, size_t DataSize)
{
    if (!Buffer || !Data)
    {
        LOG("Wrong ConstantBuffer");
        return;
    }
    PSConstantBuffers.push_back({ Slot, Buffer, Data, DataSize });
}

void FTextureRenderJob::AddTexture(uint32_t Slot, ID3D11ShaderResourceView* SRV)
{
    if (!SRV)
    {
        LOG("Wrong Texture");
        return;
    } 
    Textures.push_back({ Slot, SRV });
}

void FTextureRenderJob::AddSampler(uint32_t Slot, ID3D11SamplerState* Sampler)
{
    if (!Sampler)
    {
        LOG("Wrong Sampler");
        return;
    }
    Samplers.push_back({ Slot, Sampler });
}
