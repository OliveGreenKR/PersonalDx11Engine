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

    virtual std::vector<ConstantBufferInfo> GetVSConstantBufferInfos() const = 0;
    virtual std::vector<ConstantBufferInfo> GetPSConstantBufferInfos() const = 0;

    // 텍스처와 샘플러 정보
    struct TextureBindingInfo {
        uint32_t Slot;
        std::string Name;
    };

    struct SamplerBindingInfo {
        uint32_t Slot;
        std::string Name;
    };

    virtual std::vector<TextureBindingInfo> GetTextureInfos() const = 0;
    virtual std::vector<SamplerBindingInfo> GetSamplerInfos() const = 0;
};