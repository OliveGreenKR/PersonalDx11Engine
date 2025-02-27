#pragma once
#include "D3D.h"

// GPU ���� �ڿ� ���� Ŭ����
class FBufferResource
{
public:
    FBufferResource() = default;
    ~FBufferResource() { Release(); }

    // ���� ���� ����
    bool CreateVertexBuffer(ID3D11Device* device, const void* data, uint32_t size, bool dynamic = false) {
        Release();

        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = size;
        desc.Usage = dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        desc.CPUAccessFlags = dynamic ? D3D11_CPU_ACCESS_WRITE : 0;

        D3D11_SUBRESOURCE_DATA initData = {};
        initData.pSysMem = data;

        HRESULT hr = device->CreateBuffer(&desc, &initData, &VertexBuffer);
        return SUCCEEDED(hr);
    }

    // �ε��� ���� ����
    bool CreateIndexBuffer(ID3D11Device* device, const uint32_t* data, uint32_t count, bool dynamic = false) {
        if (!data || count == 0) return false;

        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = count * sizeof(uint32_t);
        desc.Usage = dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        desc.CPUAccessFlags = dynamic ? D3D11_CPU_ACCESS_WRITE : 0;

        D3D11_SUBRESOURCE_DATA initData = {};
        initData.pSysMem = data;

        HRESULT hr = device->CreateBuffer(&desc, &initData, &IndexBuffer);
        return SUCCEEDED(hr);
    }

    // ���� ����
    ID3D11Buffer* GetVertexBuffer() const { return VertexBuffer; }
    ID3D11Buffer* GetIndexBuffer() const { return IndexBuffer; }
    bool HasIndexBuffer() const { return IndexBuffer != nullptr; }

    // �ڿ� ����
    void Release() {
        if (VertexBuffer) {
            VertexBuffer->Release();
            VertexBuffer = nullptr;
        }

        if (IndexBuffer) {
            IndexBuffer->Release();
            IndexBuffer = nullptr;
        }
    }

private:
    ID3D11Buffer* VertexBuffer = nullptr;
    ID3D11Buffer* IndexBuffer = nullptr;
};