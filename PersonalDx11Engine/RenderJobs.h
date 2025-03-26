// RenderJob.h
#pragma once
#include "RenderStateInterface.h"
#include "Math.h"
#include <d3d11.h>
#include <vector>

// 전방 선언
class FRenderContext;

//렌더 컨텍스트에 추가로 필요한 리소스 전달 및 바인딩 execute
class FRenderJobBase
{
public:
    FRenderJobBase() = default;
    virtual ~FRenderJobBase() = default;
    virtual void Execute(FRenderContext* Context) = 0 ;
    virtual ERenderStateType GetStateType() const = 0;
};

class FTextureRenderJob : public FRenderJobBase
{
public:
    ~FTextureRenderJob() override = default;

    FTextureRenderJob() : StateType(ERenderStateType::Solid)
    { 

    }

    virtual void Execute(class FRenderContext* Context) override;

    ERenderStateType GetStateType() const override { return StateType; }

    //필요한 렌더링 컨텍스트 리소스 전달

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

    // 멤버 변수
    ERenderStateType StateType;

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
};