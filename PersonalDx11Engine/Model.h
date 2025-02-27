#pragma once
#include "VertexDataContainer.h"
#include "ModelBufferManager.h"

// ������ �� Ŭ����
class UModel
{
public:
    UModel() = default;
    ~UModel() = default;

    // �پ��� �ʱ�ȭ �ɼ�
    template<typename T>
    bool Initialize(const T* vertices, uint32_t count, const TVertexFormat<T>& format);

    // �ε��� ���� ���� �ʱ�ȭ
    template<typename T>
    bool Initialize(const T* vertices, uint32_t vertexCount,
                    const uint32_t* indices, uint32_t indexCount,
                    const TVertexFormat<T>& format);

    // �̸� ���ǵ� �������� �ʱ�ȭ
    bool InitializeAsCube();
    bool InitializeAsSphere();
    bool InitializeAsPlane();

    // ���� ����
    FBufferResource* GetBufferResource();

    bool IsInitialized() const { return bIsInitialized; }
    size_t GetDataHash() const { return DataHash; }

private:
    size_t DataHash = 0;
    bool bIsInitialized = false;
};

template<typename T>
bool UModel::Initialize(const T* vertices, uint32_t count, const TVertexFormat<T>& format) {
    auto vertexData = std::make_unique<FVertexDataContainer>(vertices, count, format);
    auto manager = UModelBufferManager::Get();

    DataHash = manager->RegisterVertexData(std::move(vertexData));
    bIsInitialized = (DataHash != 0);

    return bIsInitialized;
}

template<typename T>
bool UModel::Initialize(const T* vertices, uint32_t vertexCount,
                        const uint32_t* indices, uint32_t indexCount,
                        const TVertexFormat<T>& format) {
    auto vertexData = std::make_unique<FVertexDataContainer>(
        vertices, vertexCount, sizeof(T),
        indices, indexCount,
        format.Clone()
    );

    auto manager = UModelBufferManager::Get();
    DataHash = manager->RegisterVertexData(std::move(vertexData));
    bIsInitialized = (DataHash != 0);

    return bIsInitialized;
}


