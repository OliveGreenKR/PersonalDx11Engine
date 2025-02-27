#pragma once
#include "BufferResource.h"
#include <set>
#include <mutex>
#include <unordered_map>
#include "VertexDataContainer.h"

// ���� �Ŵ��� (Ȯ��� ����)
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

        // �̸� ���ǵ� �޽� �ʱ�ȭ
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

    // ���� ������ ���
    size_t RegisterVertexData(std::unique_ptr<FVertexDataContainer> vertexData) {
        std::lock_guard<std::mutex> lock(Mutex);

        if (!vertexData) return 0;

        size_t hash = vertexData->GetHash();

        // �̹� ��ϵ� ���������� Ȯ��
        if (VertexDataCache.find(hash) != VertexDataCache.end()) {
            return hash;
        }

        // �� ������ ���
        VertexDataCache[hash] = std::move(vertexData);

        // ���� ����
        CreateBufferForData(hash);

        return hash;
    }

    // Ű�� ���� ����
    FBufferResource* GetBufferByHash(size_t hash) {
        std::lock_guard<std::mutex> lock(Mutex);

        auto it = BufferCache.find(hash);
        if (it == BufferCache.end()) {
            return nullptr;
        }

        // LRU ������Ʈ
        UpdateLRUCache(hash);

        return &it->second;
    }

    const FVertexDataContainer* GetVertexDataByHash(size_t hash);

    // �̸� ���ǵ� �޽� ����
    size_t GetCubeHash() const { return PredefinedMeshes.count("Cube") ? PredefinedMeshes.at("Cube") : 0; }
    size_t GetSphereHash() const { return PredefinedMeshes.count("Sphere") ? PredefinedMeshes.at("Sphere") : 0; }
    size_t GetPlaneHash() const { return PredefinedMeshes.count("Plane") ? PredefinedMeshes.at("Plane") : 0; }

private:
    UModelBufferManager() = default;
    ~UModelBufferManager() { Release(); }

    // �̸� ���ǵ� �޽� �ʱ�ȭ
    void InitializePredefinedMeshes() {
        // ť�� �޽�
        auto cubeData = CreateCubeMesh();
        size_t cubeHash = RegisterVertexData(std::move(cubeData));
        PredefinedMeshes["Cube"] = cubeHash;

        // �� �޽�
        auto sphereData = CreateSphereMesh(32, 32);
        size_t sphereHash = RegisterVertexData(std::move(sphereData));
        PredefinedMeshes["Sphere"] = sphereHash;

        // ��� �޽�
        auto planeData = CreatePlaneMesh();
        size_t planeHash = RegisterVertexData(std::move(planeData));
        PredefinedMeshes["Plane"] = planeHash;
    }

    // ���� ���� ����
    bool CreateBufferForData(size_t hash) {
        if (!Device) return false;

        auto it = VertexDataCache.find(hash);
        if (it == VertexDataCache.end()) return false;

        const FVertexDataContainer* data = it->second.get();

        // ���� ĳ�ð� �ִ� �뷮�� �����ߴ��� Ȯ��
        if (BufferCache.size() >= MaxBuffers) {
            // LRU ���� ã�Ƽ� ��ü
            size_t lruHash = FindLRUBufferToReplace();
            if (lruHash != 0 && lruHash != hash) {
                BufferCache.erase(lruHash);
            }
        }

        // �� ���� ����
        FBufferResource bufferResource;
        bufferResource.CreateVertexBuffer(
            Device,
            data->GetVertexData(),
            data->GetVertexCount() * data->GetStride()
        );

        // �ε����� ������ �ε��� ���۵� ����
        if (data->GetIndexCount() > 0 && data->GetIndexData()) {
            bufferResource.CreateIndexBuffer(
                Device,
                data->GetIndexData(),
                data->GetIndexCount()
            );
        }

        // ĳ�ÿ� �߰�
        BufferCache[hash] = std::move(bufferResource);
        UpdateLRUCache(hash);

        return true;
    }

    // LRU ĳ�� ������Ʈ
    void UpdateLRUCache(size_t hash) {
        // ���� �ð� ���
        LastUsedTime[hash] = std::time(nullptr);

        // LRU ����Ʈ���� ���� �� �տ� �߰�
        auto it = std::find(LRUList.begin(), LRUList.end(), hash);
        if (it != LRUList.end()) {
            LRUList.erase(it);
        }
        LRUList.push_front(hash);
    }

    // LRU ���� ã��
    size_t FindLRUBufferToReplace() {
        // �̸� ���ǵ� �޽��� ��ü���� ����
        std::set<size_t> predefinedHashes;
        for (const auto& pair : PredefinedMeshes) {
            predefinedHashes.insert(pair.second);
        }

        // LRU ����Ʈ���� �̸� ���ǵ��� ���� ���� ������ ���� ã��
        for (auto it = LRUList.rbegin(); it != LRUList.rend(); ++it) {
            if (predefinedHashes.find(*it) == predefinedHashes.end()) {
                return *it;
            }
        }

        return 0; // ��ü�� ���� ����
    }

    // �̸� ���ǵ� �޽� ���� �Լ���
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