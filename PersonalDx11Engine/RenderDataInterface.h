#pragma once
#include <cstdint>

// 렌더 데이터 기본 인터페이스
class IRenderData
{
public:
    // 공통 필수 데이터 접근자
    virtual bool IsVisible() const = 0;

    // 선택적 리소스 접근자 (기본 구현은 nullptr 반환)
    virtual class ID3D11Buffer* GetVertexBuffer() const { return nullptr; }
    virtual class ID3D11Buffer* GetIndexBuffer() const { return nullptr; }
    virtual uint32_t GetVertexCount() const { return 0; }
    virtual uint32_t GetIndexCount() const { return 0; }
    virtual uint32_t GetStartIndex() const { return 0; }
    virtual uint32_t GetBaseVertexLocatioan() const { return 0; }
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