#include "RenderDataTexture.h"
#include "Debug.h"

void FRenderDataTexture::AddVSConstantBuffer(uint32_t Slot, ID3D11Buffer* Buffer, void* Data, size_t DataSize)
{
    if (!Buffer || !Data)
    {
        LOG("Wrong ConstantBuffer");
        return;
    }
	VSConstantBuffers.push_back({ Slot, Buffer, Data, DataSize });
}

void FRenderDataTexture::AddPSConstantBuffer(uint32_t Slot, ID3D11Buffer* Buffer, void* Data, size_t DataSize)
{
    if (!Buffer || !Data)
    {
        LOG("Wrong ConstantBuffer");
        return;
    }
    PSConstantBuffers.push_back({ Slot, Buffer, Data, DataSize });
}

void FRenderDataTexture::AddTexture(uint32_t Slot, ID3D11ShaderResourceView* SRV)
{
    if (!SRV)
    {
        LOG_FUNC_CALL("[Warning] empty Texture");
    } 
    Textures.push_back({ Slot, SRV });
}

void FRenderDataTexture::AddSampler(uint32_t Slot, ID3D11SamplerState* Sampler)
{
    if (!Sampler)
    {
        LOG_FUNC_CALL("[Warning] empty Texture");
    }
    Samplers.push_back({ Slot, Sampler });
}

bool FRenderDataTexture::IsVisible() const
{
    return bIsVisible;
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

void FRenderDataTexture::GetVSConstantBufferData(size_t Index, uint32_t& OutSlot, ID3D11Buffer*& OutBuffer, void*& OutData, size_t& OutDataSize) const
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

