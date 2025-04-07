#pragma once
#include <cstdint>
#include "FrameMemoryPool.h"

// 렌더 데이터 기본 인터페이스
class IRenderData
{
public:
    // 공통 필수 데이터 접근자
    virtual bool IsVisible() const = 0;

    //리소스 지정자
    virtual void AddPSConstantBuffer(uint32_t Slot, class ID3D11Buffer* Buffer, void* Data, size_t DataSize) = 0;
    virtual void AddVSConstantBuffer(uint32_t Slot, class ID3D11Buffer* Buffer, void* Data, size_t DataSize) = 0;
    virtual void AddTexture(uint32_t Slot, class ID3D11ShaderResourceView* SRV) = 0;
    virtual void AddSampler(uint32_t Slot, class ID3D11SamplerState* Sampler) = 0;

    //리소스 지정자
    virtual void SetVertexBuffer(class ID3D11Buffer* buffer) = 0;
    virtual void SetIndexBuffer(class ID3D11Buffer* buffer) = 0;
    virtual void SetVertexCount(uint32_t count) = 0;
    virtual void SetIndexCount(uint32_t count) = 0;
    virtual void SetStartIndex(uint32_t index) = 0;
    virtual void SetBaseVertexLocation(uint32_t location) = 0;  
    virtual void SetStride(uint32_t stride) = 0;
    virtual void SetOffset(uint32_t offset) = 0;

    // 선택적 리소스 접근자 (기본 구현은 nullptr 반환)
    virtual class ID3D11Buffer* GetVertexBuffer() const { return nullptr; }
    virtual class ID3D11Buffer* GetIndexBuffer() const { return nullptr; }
    virtual uint32_t GetVertexCount() const { return 0; }
    virtual uint32_t GetIndexCount() const { return 0; }
    virtual uint32_t GetStartIndex() const { return 0; }
    virtual uint32_t GetBaseVertexLocation() const { return 0; }
    virtual uint32_t GetStride() const { return 0; }
    virtual uint32_t GetOffset() const { return 0; }

    // 선택적 텍스처/샘플러 데이터
    virtual size_t GetTextureCount() const { return 0; }
    virtual void GetTextureData(size_t Index, uint32_t& OutSlot, class ID3D11ShaderResourceView*& OutSRV) const {}

    // 선택적 상수 버퍼 데이터
    virtual size_t GetVSConstantBufferCount() const { return 0; }
    virtual void GetVSConstantBufferData(size_t Index, uint32_t& OutSlot, ID3D11Buffer*& OutBuffer, void*& OutData, size_t& OutDataSize) const {}

    // 선택적 상수 버퍼 데이터
    virtual size_t GetPSConstantBufferCount() const { return 0; }
    virtual void GetPSConstantBufferData(size_t Index, uint32_t& OutSlot, ID3D11Buffer*& OutBuffer, void*& OutData, size_t& OutDataSize) const {}
};