#pragma once
#include "D3D.h"
#include "VertexDataContainer.h"
#include <unordered_map>
#include <memory>
#include <vector>
#include <string>

// 버퍼 리소스 클래스 (정점 버퍼와 인덱스 버퍼를 함께 관리)
class FBufferResource
{
public:
    FBufferResource() = default;
    ~FBufferResource();

    // 버퍼 초기화 메서드
    bool Initialize(ID3D11Device* InDevice, const FVertexDataContainer& InVertexData);
    void Release();

    // 게터 메서드
    ID3D11Buffer* GetVertexBuffer() const { return VertexBuffer; }
    ID3D11Buffer* GetIndexBuffer() const { return IndexBuffer; }
    UINT GetVertexCount() const { return VertexCount; }
    UINT GetIndexCount() const { return IndexCount; }
    UINT GetStride() const { return Stride; }
    UINT GetOffset() const { return Offset; }
    size_t GetLastAccessTick() const { return LastAccessTick; }

    // 마지막 접근 시간 업데이트
    void UpdateAccessTick(size_t InTick) { LastAccessTick = InTick; }

private:
    ID3D11Buffer* VertexBuffer = nullptr;
    ID3D11Buffer* IndexBuffer = nullptr;
    UINT VertexCount = 0;
    UINT IndexCount = 0;
    UINT Stride = 0;
    UINT Offset = 0;
    size_t LastAccessTick = 0; // LRU 알고리즘에 사용될 마지막 접근 시간
};

// 모델 버퍼 매니저 클래스 (싱글톤 패턴)
class UModelBufferManager
{
public:
    // 싱글톤 인스턴스 접근자
    static UModelBufferManager* Get()
    {
        // 싱글톤 인스턴스
        static UModelBufferManager* Instance = new UModelBufferManager();
        if (!Instance->bInitialized)
        {
            Instance->Initialize();
        }
        return Instance;
    }

    // 디바이스 설정
    void SetDevice(ID3D11Device* InDevice) { Device = InDevice; }

    // 기본 프리미티브 모델 해시 접근자
    size_t GetCubeHash() const { return CubeModelHash; }
    size_t GetSphereHash() const { return SphereModelHash; }
    size_t GetPlaneHash() const { return PlaneModelHash; }

    // 초기화 및 해제
    bool Initialize();
    void Release();

    // 버퍼 리소스 접근자
    FBufferResource* GetBufferByHash(size_t InHash);

    // 정점 데이터로부터 해시 생성 또는 접근
    size_t RegisterVertexData(const FVertexDataContainer& InVertexData);

    // 해시로부터 정점 데이터 접근
    const FVertexDataContainer* GetVertexDataByHash(size_t InHash) const;

private:
    UModelBufferManager() = default;
    ~UModelBufferManager();

    bool bInitialized = false;

    // 기본 프리미티브 모델 생성 메서드
    void CreateDefaultPrimitives();
    FVertexDataContainer CreateCubeVertexData();
    FVertexDataContainer CreateSphereVertexData(int InSegments = 32);
    FVertexDataContainer CreatePlaneVertexData();

    // 해시 계산 헬퍼 메서드
    size_t CalculateHash(const FVertexDataContainer& InVertexData) const;

    //LRU 비교 함수 (Is A more Recently used than B)
    bool IsMoreRecentlyUsed(size_t tickA, size_t tickB);

    // 버퍼 풀 관리 메서드
    bool AddBufferToPool(size_t InHash, const FVertexDataContainer& InVertexData);
    void ReplaceBufferInPool();

    // 멤버 변수
    ID3D11Device* Device = nullptr;
    //클수록 최근에 사용된것
    size_t CurrentTick = 0;

    // 기본 프리미티브 모델 해시
    size_t CubeModelHash = 0;
    size_t SphereModelHash = 0;
    size_t PlaneModelHash = 0;

    // 풀 관리 상수
    const size_t MAX_BUFFER_POOL_SIZE = 100;

    // 정점 데이터 컨테이너 및 버퍼 풀 맵
    std::unordered_map<size_t, FVertexDataContainer> VertexDataMap;
    std::unordered_map<size_t, std::unique_ptr<FBufferResource>> BufferPool;

    // 기본 모델 플래그 (기본 모델은 LRU에서 제외)
    std::unordered_map<size_t, bool> DefaultModelFlags;
};