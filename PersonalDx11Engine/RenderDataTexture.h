#pragma once
#include "RenderDataSimpleColor.h"


class FRenderDataTexture : public FRenderDataSimpleColor
{
public:
    FRenderDataTexture() = default;
    virtual ~FRenderDataTexture() = default;

    void AddPSConstantBuffer(uint32_t Slot, ID3D11Buffer* Buffer, void* Data, size_t DataSize) override;
    void AddTexture(uint32_t Slot, ID3D11ShaderResourceView* SRV) override;
    void AddSampler(uint32_t Slot, ID3D11SamplerState* Sampler) override;

protected:
    struct TextureBindData {
        uint32_t Slot;
        ID3D11ShaderResourceView* SRV; 
    };

    struct SamplerBindData {
        uint32_t Slot;
        ID3D11SamplerState* Sampler;
    };

  
    std::vector<ConstantBufferBindData> PSConstantBuffers = std::vector<ConstantBufferBindData>();
    std::vector<TextureBindData> Textures = std::vector<TextureBindData>();
    std::vector<SamplerBindData> Samplers = std::vector<SamplerBindData>();  

public:
    // 선택적 텍스처 데이터
    size_t GetTextureCount() const { return Textures.size(); }
    void GetTextureData(size_t Index, uint32_t& OutSlot, class ID3D11ShaderResourceView*& OutSRV) const;

    virtual size_t GetPSConstantBufferCount() const { return PSConstantBuffers.size(); }
    virtual void GetPSConstantBufferData(size_t Index, uint32_t& OutSlot, ID3D11Buffer*& OutBuffer, void*& OutData, size_t& OutDataSize) const;
};