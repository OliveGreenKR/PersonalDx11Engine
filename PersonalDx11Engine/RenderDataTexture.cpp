#include "RenderDataTexture.h"
#include "Debug.h"

void FRenderDataTexture::AddPSConstantBuffer(uint32_t Slot, ID3D11Buffer* Buffer, void* Data, size_t DataSize)
{
    if (!Buffer)
    {
        LOG("Wrong ConstantBuffer");
        return;
    }
    //null도 전달
    PSConstantBuffers.push_back({ Slot, Buffer, Data, DataSize });
}

void FRenderDataTexture::AddTexture(uint32_t Slot, ID3D11ShaderResourceView* SRV)
{
    //null도 전달
    Textures.push_back({ Slot, SRV });
}

void FRenderDataTexture::AddSampler(uint32_t Slot, ID3D11SamplerState* Sampler)
{
    //null도 전달
    Samplers.push_back({ Slot, Sampler });
}

void FRenderDataTexture::GetTextureData(size_t Index, uint32_t& OutSlot, ID3D11ShaderResourceView*& OutSRV) const
{
    //Out 초기화
    OutSlot = -1; 
    OutSRV = nullptr;

    if (Index >= Textures.size())
        return;

	OutSlot = Textures[Index].Slot;
    OutSRV = Textures[Index].SRV;

    return;
}

void FRenderDataTexture::GetPSConstantBufferData(size_t Index, uint32_t& OutSlot, ID3D11Buffer*& OutBuffer, void*& OutData, size_t& OutDataSize) const
{
    //Out 초기화
    OutSlot = -1;
    OutDataSize = 0;
    OutBuffer = nullptr;
    OutData = nullptr;

    if (Index >= PSConstantBuffers.size())
        return;


    OutSlot = PSConstantBuffers[Index].Slot;
    OutBuffer = PSConstantBuffers[Index].Buffer;
    OutData = PSConstantBuffers[Index].Data;
    OutDataSize = PSConstantBuffers[Index].DataSize;
    return;
}

