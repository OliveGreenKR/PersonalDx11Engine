#pragma once
#include "VertexDataContainer.h"
#include "ResourceInterface.h"
#include <cstdint>
class ID3D11Device;
class ID3D11Buffer;

//모델 리소스(정점, 인덱스 데이터)
class UModel :  public IResource
{
public:
    UModel() = default;
    ~UModel();

    // 버퍼 생성
    bool CreateBuffers(class ID3D11Device* InDevice, const FVertexDataContainer& InVertexData);
    void Release();

    // 게터 메서드
    ID3D11Buffer* GetVertexBuffer() const { return VertexBuffer; }
    ID3D11Buffer* GetIndexBuffer() const { return IndexBuffer; }
    uint32_t GetVertexCount() const { return VertexCount; }
    uint32_t GetIndexCount() const { return IndexCount; }
    uint32_t GetStride() const { return Stride; }
    uint32_t GetOffset() const { return Offset; } 
      
    // Inherited via IResource
    bool Load(IRenderHardware* RenderHardware, const std::wstring& Path) override;
    bool LoadAsync(IRenderHardware* RenderHardware, const std::wstring& Path) override;
    bool IsLoaded() const override         { return bIsLoaded; } 
    size_t GetMemorySize() const override;
    EResourceType GetType() const override { return EResourceType::Model; }
private:
    static FVertexDataContainer CreateCubeVertexData();
    static FVertexDataContainer CreateSphereVertexData(int InSegments = 32);
    static FVertexDataContainer CreatePlaneVertexData();
private:
    ID3D11Buffer* VertexBuffer = nullptr;
    ID3D11Buffer* IndexBuffer = nullptr;
    uint32_t VertexCount = 0;
    uint32_t IndexCount = 0;
    uint32_t Stride = 0;
    uint32_t Offset = 0;

    bool bIsLoaded = false;
};

