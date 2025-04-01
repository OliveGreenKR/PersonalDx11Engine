#pragma once
#include "Math.h"
#include <d3d11.h>
#include <vector>
#include "RenderDataInterface.h"

class FRenderDataTexture : public IRenderData
{
public:
    FRenderDataTexture() = default;
    virtual ~FRenderDataTexture() = default;

    //데이터 추가 메소드들
    void AddVSConstantBuffer(uint32_t Slot, ID3D11Buffer* Buffer, void* Data, size_t DataSize);

    void AddPSConstantBuffer(uint32_t Slot, ID3D11Buffer* Buffer, void* Data, size_t DataSize);

    void AddTexture(uint32_t Slot, ID3D11ShaderResourceView* SRV);

    void AddSampler(uint32_t Slot, ID3D11SamplerState* Sampler);

    // 데이터 구조
    struct ConstantBufferBindData {
        uint32_t Slot;
        ID3D11Buffer* Buffer;
        void* Data;
        size_t DataSize;
    };

    struct TextureBindData {
        uint32_t Slot;
        ID3D11ShaderResourceView* SRV; 
    };

    struct SamplerBindData {
        uint32_t Slot;
        ID3D11SamplerState* Sampler;
    };

    //visible
    bool bIsVisible = true;

    // 정점 버퍼 관련
    ID3D11Buffer* VertexBuffer = nullptr;
    ID3D11Buffer* IndexBuffer = nullptr;
    uint32_t VertexCount = 0;
    uint32_t IndexCount = 0;
    uint32_t Stride = 0;
    uint32_t Offset = 0;
    uint32_t StartVertex = 0; // 드로우 시작 위치
    int32_t BaseVertex = 0;   // 인덱스 드로우의 베이스 버텍스
    uint32_t StartIndex = 0;  // 인덱스 시작 위치

    // 리소스
    std::vector<ConstantBufferBindData> VSConstantBuffers = std::vector<ConstantBufferBindData>();
    std::vector<ConstantBufferBindData> PSConstantBuffers = std::vector<ConstantBufferBindData>();
    std::vector<TextureBindData> Textures = std::vector<TextureBindData>();
    std::vector<SamplerBindData> Samplers = std::vector<SamplerBindData>();

    //------------Inherited via IRenderData-----------------------------------

    bool IsVisible() const override;

    // 선택적 리소스 접근자 (기본 구현은 nullptr 반환)
    class ID3D11Buffer* GetVertexBuffer() const override { return VertexBuffer; }
    class ID3D11Buffer* GetIndexBuffer() const override { return IndexBuffer; }
    uint32_t GetVertexCount() const override { return VertexCount; }
    uint32_t GetIndexCount() const override { return IndexCount; }
    uint32_t GetStride() const override { return Stride; }
    uint32_t GetOffset() const override { return Offset; }


    // 선택적 텍스처 데이터
    size_t GetTextureCount() const { return Textures.size(); }
    void GetTextureData(size_t Index, uint32_t& OutSlot, class ID3D11ShaderResourceView*& OutSRV) const;

    // 선택적 상수 버퍼 데이터
    size_t GetVSConstantBufferCount() const { return VSConstantBuffers.size(); }
    void GetVSConstantBufferData(size_t Index, uint32_t& OutSlot, ID3D11Buffer*& OutBuffer, void*& OutData, size_t& OutDataSize) const;

    virtual size_t GetPSConstantBufferCount() const { return PSConstantBuffers.size(); }
    virtual void GetPSConstantBufferData(size_t Index, uint32_t& OutSlot, ID3D11Buffer*& OutBuffer, void*& OutData, size_t& OutDataSize) const;
};