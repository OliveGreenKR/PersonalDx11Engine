#pragma once
#include "BufferResource.h"
#include <set>
#include <mutex>
#include <unordered_map>
#include "VertexDataContainer.h"

// 버퍼 매니저 (확장된 버전)
class UModelBufferManager
{
public:
    static UModelBufferManager* Get() {
        static UModelBufferManager instance;
        return &instance;
    }

    void Initialize(ID3D11Device* device, uint32_t maxBuffers = 1024) {
        Device = device;
        MaxBuffers = maxBuffers;

        // 미리 정의된 메쉬 초기화
        InitializePredefinedMeshes();
    }

    void Release() {
        std::lock_guard<std::mutex> lock(Mutex);

        for (auto& pair : BufferCache) {
            pair.second.Release();
        }

        BufferCache.clear();
        VertexDataCache.clear();
        LRUList.clear();
        LastUsedTime.clear();

        PredefinedMeshes.clear();
        Device = nullptr;
    }

    // 정점 데이터 등록
    size_t RegisterVertexData(std::unique_ptr<FVertexDataContainer> vertexData) {
        std::lock_guard<std::mutex> lock(Mutex);

        if (!vertexData) return 0;

        size_t hash = vertexData->GetHash();

        // 이미 등록된 데이터인지 확인
        if (VertexDataCache.find(hash) != VertexDataCache.end()) {
            return hash;
        }

        // 새 데이터 등록
        VertexDataCache[hash] = std::move(vertexData);

        // 버퍼 생성
        CreateBufferForData(hash);

        return hash;
    }

    // 키로 버퍼 접근
    FBufferResource* GetBufferByHash(size_t hash) {
        std::lock_guard<std::mutex> lock(Mutex);

        auto it = BufferCache.find(hash);
        if (it == BufferCache.end()) {
            return nullptr;
        }

        // LRU 업데이트
        UpdateLRUCache(hash);

        return &it->second;
    }

    const FVertexDataContainer* GetVertexDataByHash(size_t hash);

    // 미리 정의된 메쉬 접근
    size_t GetCubeHash() const { return PredefinedMeshes.count("Cube") ? PredefinedMeshes.at("Cube") : 0; }
    size_t GetSphereHash() const { return PredefinedMeshes.count("Sphere") ? PredefinedMeshes.at("Sphere") : 0; }
    size_t GetPlaneHash() const { return PredefinedMeshes.count("Plane") ? PredefinedMeshes.at("Plane") : 0; }

private:
    UModelBufferManager() = default;
    ~UModelBufferManager() { Release(); }

    // 미리 정의된 메쉬 초기화
    void InitializePredefinedMeshes() {
        // 큐브 메쉬
        auto cubeData = CreateCubeMesh();
        size_t cubeHash = RegisterVertexData(std::move(cubeData));
        PredefinedMeshes["Cube"] = cubeHash;

        // 구 메쉬
        auto sphereData = CreateSphereMesh(32, 32);
        size_t sphereHash = RegisterVertexData(std::move(sphereData));
        PredefinedMeshes["Sphere"] = sphereHash;

        // 평면 메쉬
        auto planeData = CreatePlaneMesh();
        size_t planeHash = RegisterVertexData(std::move(planeData));
        PredefinedMeshes["Plane"] = planeHash;
    }

    // 버퍼 생성 로직
    bool CreateBufferForData(size_t hash) {
        if (!Device) return false;

        auto it = VertexDataCache.find(hash);
        if (it == VertexDataCache.end()) return false;

        const FVertexDataContainer* data = it->second.get();

        // 버퍼 캐시가 최대 용량에 도달했는지 확인
        if (BufferCache.size() >= MaxBuffers) {
            // LRU 버퍼 찾아서 교체
            size_t lruHash = FindLRUBufferToReplace();
            if (lruHash != 0 && lruHash != hash) {
                BufferCache.erase(lruHash);
            }
        }

        // 새 버퍼 생성
        FBufferResource bufferResource;
        bufferResource.CreateVertexBuffer(
            Device,
            data->GetVertexData(),
            data->GetVertexCount() * data->GetStride()
        );

        // 인덱스가 있으면 인덱스 버퍼도 생성
        if (data->GetIndexCount() > 0 && data->GetIndexData()) {
            bufferResource.CreateIndexBuffer(
                Device,
                data->GetIndexData(),
                data->GetIndexCount()
            );
        }

        // 캐시에 추가
        BufferCache[hash] = std::move(bufferResource);
        UpdateLRUCache(hash);

        return true;
    }

    // LRU 캐시 업데이트
    void UpdateLRUCache(size_t hash) {
        // 현재 시간 기록
        LastUsedTime[hash] = std::time(nullptr);

        // LRU 리스트에서 제거 후 앞에 추가
        auto it = std::find(LRUList.begin(), LRUList.end(), hash);
        if (it != LRUList.end()) {
            LRUList.erase(it);
        }
        LRUList.push_front(hash);
    }

    // LRU 버퍼 찾기
    size_t FindLRUBufferToReplace() {
        // 미리 정의된 메쉬는 교체하지 않음
        std::set<size_t> predefinedHashes;
        for (const auto& pair : PredefinedMeshes) {
            predefinedHashes.insert(pair.second);
        }

        // LRU 리스트에서 미리 정의되지 않은 가장 오래된 버퍼 찾기
        for (auto it = LRUList.rbegin(); it != LRUList.rend(); ++it) {
            if (predefinedHashes.find(*it) == predefinedHashes.end()) {
                return *it;
            }
        }

        return 0; // 대체할 버퍼 없음
    }

    // 미리 정의된 메쉬 생성 함수들
    std::unique_ptr<FVertexDataContainer> CreateCubeMesh();
    std::unique_ptr<FVertexDataContainer> CreateSphereMesh(uint32_t slices, uint32_t stacks);
    std::unique_ptr<FVertexDataContainer> CreatePlaneMesh();

private:
    ID3D11Device* Device = nullptr;
    uint32_t MaxBuffers = 1024;

    std::unordered_map<size_t, std::unique_ptr<FVertexDataContainer>> VertexDataCache;
    std::unordered_map<size_t, FBufferResource> BufferCache;
    std::unordered_map<size_t, std::time_t> LastUsedTime;
    std::unordered_map<std::string, size_t> PredefinedMeshes;
    std::list<size_t> LRUList;

    std::mutex Mutex;
};