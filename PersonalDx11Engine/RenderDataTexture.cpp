#include "RenderDataTexture.h"
#include "Debug.h"

void FTextureRenderData::AddVSConstantBuffer(uint32_t Slot, ID3D11Buffer* Buffer, void* Data, size_t DataSize)
{
    if (!Buffer || !Data)
    {
        LOG("Wrong ConstantBuffer");
        return;
    }
	VSConstantBuffers.push_back({ Slot, Buffer, Data, DataSize });
}

void FTextureRenderData::AddPSConstantBuffer(uint32_t Slot, ID3D11Buffer* Buffer, void* Data, size_t DataSize)
{
    if (!Buffer || !Data)
    {
        LOG("Wrong ConstantBuffer");
        return;
    }
    PSConstantBuffers.push_back({ Slot, Buffer, Data, DataSize });
}

void FTextureRenderData::AddTexture(uint32_t Slot, ID3D11ShaderResourceView* SRV)
{
    if (!SRV)
    {
        LOG("Wrong Texture");
        return;
    } 
    Textures.push_back({ Slot, SRV });
}

void FTextureRenderData::AddSampler(uint32_t Slot, ID3D11SamplerState* Sampler)
{
    if (!Sampler)
    {
        LOG("Wrong Sampler");
        return;
    }
    Samplers.push_back({ Slot, Sampler });
}

bool FTextureRenderData::IsVisible() const
{
    return bIsVisible;
}

void FTextureRenderData::GetTextureData(size_t Index, uint32_t& OutSlot, ID3D11ShaderResourceView*& OutSRV) const
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

void FTextureRenderData::GetVSConstantBufferData(size_t Index, uint32_t& OutSlot, ID3D11Buffer*& OutBuffer, void*& OutData, size_t& OutDataSize) const
{
    //Out 초기화
    OutSlot = -1;
    OutDataSize = 0;
    OutBuffer = nullptr;
    OutData = nullptr;

    if (Index >= VSConstantBuffers.size())
        return;


    OutSlot = VSConstantBuffers[Index].Slot;
    OutBuffer = VSConstantBuffers[Index].Buffer;
    OutData = VSConstantBuffers[Index].Data;
    OutDataSize = VSConstantBuffers[Index].DataSize;
    return;
}

void FTextureRenderData::GetPSConstantBufferData(size_t Index, uint32_t& OutSlot, ID3D11Buffer*& OutBuffer, void*& OutData, size_t& OutDataSize) const
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

