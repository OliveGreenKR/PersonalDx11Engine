#pragma once
#include "VertexDataContainer.h"
#include "ModelBufferManager.h"

// 개선된 모델 클래스
class UModel
{
public:
    UModel() = default;
    ~UModel() = default;

    // 다양한 초기화 옵션
    template<typename T>
    bool Initialize(const T* vertices, uint32_t count, const TVertexFormat<T>& format);

    // 인덱스 버퍼 포함 초기화
    template<typename T>
    bool Initialize(const T* vertices, uint32_t vertexCount,
                    const uint32_t* indices, uint32_t indexCount,
                    const TVertexFormat<T>& format);

    // 미리 정의된 형상으로 초기화
    bool InitializeAsCube();
    bool InitializeAsSphere();
    bool InitializeAsPlane();

    // 정보 접근
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


