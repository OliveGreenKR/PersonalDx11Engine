#pragma once
#include "ModelBufferManager.h"

class UModel
{
public:
    UModel() = default;
    ~UModel() = default;

    // 기본 프리미티브 모델 초기화 메서드
    bool InitializeAsCube();
    bool InitializeAsSphere();
    bool InitializeAsPlane();

    // 커스텀 정점 데이터로 초기화하는 메서드
    bool InitializeFromVertexData(const FVertexDataContainer& InVertexData)
    {
        if (InVertexData.Vertices.empty())
            return false;

        DataHash = UModelBufferManager::Get()->RegisterVertexData(InVertexData);
        bIsInitialized = (DataHash != 0);
        return bIsInitialized;
    }

    // 버퍼 리소스 접근자
    FBufferResource* GetBufferResource();

    // 초기화 여부 확인
    bool IsInitialized() const { return bIsInitialized; }

private:
    size_t DataHash = 0;           // 정점 데이터의 해시 (모델 식별자)
    bool bIsInitialized = false;   // 초기화 여부 플래그
};