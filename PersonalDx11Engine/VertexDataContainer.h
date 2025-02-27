#pragma once
#include "VertexFormat.h"
#include <string>
#include <vector>

// ������ ���� ������ �����̳� (�پ��� ���� ���� ����)
class FVertexDataContainer
{
public:
    // �پ��� ���� ������ �����ϴ� ������
    template<typename T>
    FVertexDataContainer(const T* vertices, uint32_t count, const TVertexFormat<T>& format)
        : VertexCount(count)
        , Format(format.Clone())
    {
        // ������ ����
        Data.resize(count * sizeof(T));
        memcpy(Data.data(), vertices, Data.size());

        // �ؽ� ���
        CalculateHash();
    }

    // �ε��� ���� �߰� ����
    FVertexDataContainer(const void* vertices, uint32_t vertexCount, uint32_t stride,
                         const uint32_t* indices, uint32_t indexCount,
                         std::unique_ptr<IVertexFormat> format)
        : VertexCount(vertexCount)
        , IndexCount(indexCount)
        , Format(std::move(format))
    {
        // ���� ������ ����
        Data.resize(vertexCount * stride);
        memcpy(Data.data(), vertices, Data.size());

        // �ε��� �����Ͱ� �ִٸ� ����
        if (indices && indexCount > 0) {
            Indices.resize(indexCount);
            memcpy(Indices.data(), indices, indexCount * sizeof(uint32_t));
        }

        // �ؽ� ���
        CalculateHash();
    }

    // �ؽ� �� ������ ����
    size_t GetHash() const { return Hash; }
    const void* GetVertexData() const { return Data.data(); }
    const uint32_t* GetIndexData() const { return Indices.empty() ? nullptr : Indices.data(); }
    uint32_t GetVertexCount() const { return VertexCount; }
    uint32_t GetIndexCount() const { return IndexCount; }
    uint32_t GetStride() const { return Format->GetStride(); }

public:
    ID3D11InputLayout* GetOrCreateInputLayout(ID3D11Device* device, const void* shaderBytecode, size_t bytecodeLength) const {
        // �� �����̳� �ν��Ͻ��� ������ ���� ������ �����Ƿ�
        // �ش� ���Ŀ� ���� �Է� ���̾ƿ��� �ʿ���
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
    // �Է� ���̾ƿ� ���� ����
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
        // ���� �����Ϳ� �ε��� �����͸� ��� ����Ͽ� �ؽ� ���
        Hash = std::hash<std::string>{}(std::string(
            reinterpret_cast<const char*>(Data.data()),
            Data.size()
        ));

        if (!Indices.empty()) {
            size_t indexHash = std::hash<std::string>{}(std::string(
                reinterpret_cast<const char*>(Indices.data()),
                Indices.size() * sizeof(uint32_t)
            ));

            // �ؽ� ����
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
