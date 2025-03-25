#include "RenderJobs.h"
#include "ShaderInterface.h"
#include "RenderContext.h"

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
        
    // 2. 쉐이더 바인딩
    if (VertexShader && PixelShader && InputLayout)
    {
        Context->BindShader(VertexShader, PixelShader, InputLayout);
    }
        

    // 3. 상수 버퍼 바인딩 
    for (const auto& CB : VSConstantBuffers)
    {
        Context->BindConstantBuffer(CB.Slot, CB.Buffer, nullptr, 0, true);
    }
    for (const auto& CB : PSConstantBuffers)
    {
        Context->BindConstantBuffer(CB.Slot, CB.Buffer, nullptr, 0, false);
    }
        

    // 4. 텍스처 및 샘플러 바인딩
    for (const auto& Tex : Textures)
    {
        Context->BindShaderResource(Tex.Slot, Tex.SRV);
    }

    for (const auto& Samp : Samplers)
    {
        Context->BindSamplerState(Samp.Slot, Samp.Sampler);
    }
        

    // 5. 드로우 콜 실행
    if (IndexCount > 0)
    {
        Context->DrawIndexed(IndexCount, StartIndex, BaseVertex);
    } 
    else if (VertexCount > 0)
    {
        Context->Draw(VertexCount, StartVertex);
    }
        
}

void FTextureRenderJob::SetShaderResources(IShader* InShader)
{
    if (!InShader)
        return;

    VertexShader = InShader->GetVertexShader();
    PixelShader = InShader->GetPixelShader();
    InputLayout = InShader->GetInputLayout();

    // VS 상수 버퍼
    auto vsCBs = InShader->GetVSConstantBuffers();
    VSConstantBuffers.resize(vsCBs.size()); 
    for (size_t i = 0; i < vsCBs.size(); ++i)
    {
        VSConstantBuffers[i].Slot = vsCBs[i].Slot;
        VSConstantBuffers[i].Buffer = vsCBs[i].Buffer;
        VSConstantBuffers[i].DataSize = vsCBs[i].Size;
    }

    // PS 상수 버퍼
    auto psCBs = InShader->GetPSConstantBuffers();
    PSConstantBuffers.resize(psCBs.size());
    for (size_t i = 0; i < psCBs.size(); ++i)
    {
        PSConstantBuffers[i].Slot = psCBs[i].Slot;
        PSConstantBuffers[i].Buffer = psCBs[i].Buffer;
        PSConstantBuffers[i].DataSize = psCBs[i].Size;
    }

    // 텍스처
    auto textures = InShader->GetTextures();
    Textures.resize(textures.size());
    for (size_t i = 0; i < textures.size(); ++i)
    {
        Textures[i].Slot = textures[i].Slot;
        Textures[i].SRV = textures[i].SRV;
    }


    // 샘플러
    auto samplers = InShader->GetSamplers();;
    Samplers.resize(samplers.size());
    for (size_t i = 0; i < samplers.size(); ++i)
    {
        Samplers[i].Slot = samplers[i].Slot;
        Samplers[i].Sampler = samplers[i].Sampler;
    }
}

void FTextureRenderJob::AddVSConstantBuffer(uint32_t Slot, ID3D11Buffer* Buffer, void* Data, size_t DataSize)
{
    VSConstantBuffers.push_back({ Slot, Buffer, Data, DataSize });
}

inline void FTextureRenderJob::AddPSConstantBuffer(uint32_t Slot, ID3D11Buffer* Buffer)
{
    PSConstantBuffers.push_back({ Slot, Buffer });
}

inline void FTextureRenderJob::AddTexture(uint32_t Slot, ID3D11ShaderResourceView* SRV)
{
    Textures.push_back({ Slot, SRV });
}

inline void FTextureRenderJob::AddSampler(uint32_t Slot, ID3D11SamplerState* Sampler)
{
    Samplers.push_back({ Slot, Sampler });
}
