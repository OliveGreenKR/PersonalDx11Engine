#pragma once
#include "VertexFormat.h"
#include <string>
#include <vector>

// 개선된 정점 데이터 컨테이너 (다양한 정점 형식 지원)
class FVertexDataContainer
{
public:
    // 다양한 정점 형식을 지원하는 생성자
    template<typename T>
    FVertexDataContainer(const T* vertices, uint32_t count, const TVertexFormat<T>& format)
        : VertexCount(count)
        , Format(format.Clone())
    {
        // 데이터 복사
        Data.resize(count * sizeof(T));
        memcpy(Data.data(), vertices, Data.size());

        // 해시 계산
        CalculateHash();
    }

    // 인덱스 버퍼 추가 지원
    FVertexDataContainer(const void* vertices, uint32_t vertexCount, uint32_t stride,
                         const uint32_t* indices, uint32_t indexCount,
                         std::unique_ptr<IVertexFormat> format)
        : VertexCount(vertexCount)
        , IndexCount(indexCount)
        , Format(std::move(format))
    {
        // 정점 데이터 복사
        Data.resize(vertexCount * stride);
        memcpy(Data.data(), vertices, Data.size());

        // 인덱스 데이터가 있다면 복사
        if (indices && indexCount > 0) {
            Indices.resize(indexCount);
            memcpy(Indices.data(), indices, indexCount * sizeof(uint32_t));
        }

        // 해시 계산
        CalculateHash();
    }

    // 해시 및 데이터 접근
    size_t GetHash() const { return Hash; }
    const void* GetVertexData() const { return Data.data(); }
    const uint32_t* GetIndexData() const { return Indices.empty() ? nullptr : Indices.data(); }
    uint32_t GetVertexCount() const { return VertexCount; }
    uint32_t GetIndexCount() const { return IndexCount; }
    uint32_t GetStride() const { return Format->GetStride(); }

public:
    ID3D11InputLayout* GetOrCreateInputLayout(ID3D11Device* device, const void* shaderBytecode, size_t bytecodeLength) const {
        // 각 컨테이너 인스턴스는 동일한 정점 형식을 가지므로
        // 해당 형식에 대한 입력 레이아웃만 필요함
        static ID3D11InputLayout* cachedLayout = nullptr;
        static bool initialized = false;

        if (!initialized) {
            HRESULT hr = device->CreateInputLayout(
                Format->GetInputLayoutDesc(),
                Format->GetInputLayoutElementCount(),
                shaderBytecode,
                bytecodeLength,
                &cachedLayout
            );

            initialized = SUCCEEDED(hr);
        }

        return cachedLayout;
    }

public:
    // 입력 레이아웃 생성 지원
    HRESULT CreateInputLayout(ID3D11Device* device, const void* shaderBytecode,
                              size_t bytecodeLength, ID3D11InputLayout** inputLayout) const
    {
        return device->CreateInputLayout(
            Format->GetInputLayoutDesc(),
            Format->GetInputLayoutElementCount(),
            shaderBytecode,
            bytecodeLength,
            inputLayout
        );
    }


private:
    void CalculateHash() {
        // 정점 데이터와 인덱스 데이터를 모두 고려하여 해시 계산
        Hash = std::hash<std::string>{}(std::string(
            reinterpret_cast<const char*>(Data.data()),
            Data.size()
        ));

        if (!Indices.empty()) {
            size_t indexHash = std::hash<std::string>{}(std::string(
                reinterpret_cast<const char*>(Indices.data()),
                Indices.size() * sizeof(uint32_t)
            ));

            // 해시 결합
            Hash = Hash ^ (indexHash + 0x9e3779b9 + (Hash << 6) + (Hash >> 2));
        }
    }

    std::vector<uint8_t> Data;
    std::vector<uint32_t> Indices;
    uint32_t VertexCount = 0;
    uint32_t IndexCount = 0;
    std::unique_ptr<IVertexFormat> Format;
    size_t Hash = 0;
};
