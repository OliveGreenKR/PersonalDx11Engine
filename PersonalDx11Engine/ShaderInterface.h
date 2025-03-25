// IShader.h
#pragma once
#include <d3d11.h>
#include <vector>
#include <string>

class IShader
{
public:
    virtual ~IShader() = default;

    // 기본 쉐이더 리소스
    virtual ID3D11VertexShader* GetVertexShader() const = 0;
    virtual ID3D11PixelShader* GetPixelShader() const = 0;
    virtual ID3D11InputLayout* GetInputLayout() const = 0;

    // 상수 버퍼 정보
    struct ConstantBufferInfo {
        uint32_t Slot;
        ID3D11Buffer* Buffer;
        uint32_t Size;
        std::string Name;
    };

    virtual std::vector<ConstantBufferInfo> GetVSConstantBuffers() const = 0;
    virtual std::vector<ConstantBufferInfo> GetPSConstantBuffers() const = 0;

    // 텍스처와 샘플러 정보
    struct TextureBinding {
        uint32_t Slot;
        ID3D11ShaderResourceView* SRV;
        std::string Name;
    };

    struct SamplerBinding {
        uint32_t Slot;
        ID3D11SamplerState* Sampler;
        std::string Name;
    };


    virtual std::vector<TextureBinding> GetTextures() const = 0;
    virtual std::vector<SamplerBinding> GetSamplers() const = 0;
};