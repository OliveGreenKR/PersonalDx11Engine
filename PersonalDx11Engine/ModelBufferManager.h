#pragma once
#include "D3D.h"
#include "VertexDataContainer.h"
#include <unordered_map>
#include <memory>
#include <vector>
#include <string>

// ���� ���ҽ� Ŭ���� (���� ���ۿ� �ε��� ���۸� �Բ� ����)
class FBufferResource
{
public:
    FBufferResource() = default;
    ~FBufferResource();

    // ���� �ʱ�ȭ �޼���
    bool Initialize(ID3D11Device* InDevice, const FVertexDataContainer& InVertexData);
    void Release();

    // ���� �޼���
    ID3D11Buffer* GetVertexBuffer() const { return VertexBuffer; }
    ID3D11Buffer* GetIndexBuffer() const { return IndexBuffer; }
    UINT GetVertexCount() const { return VertexCount; }
    UINT GetIndexCount() const { return IndexCount; }
    UINT GetStride() const { return Stride; }
    UINT GetOffset() const { return Offset; }
    size_t GetLastAccessTick() const { return LastAccessTick; }

    // ������ ���� �ð� ������Ʈ
    void UpdateAccessTick(size_t InTick) { LastAccessTick = InTick; }

private:
    ID3D11Buffer* VertexBuffer = nullptr;
    ID3D11Buffer* IndexBuffer = nullptr;
    UINT VertexCount = 0;
    UINT IndexCount = 0;
    UINT Stride = 0;
    UINT Offset = 0;
    size_t LastAccessTick = 0; // LRU �˰��� ���� ������ ���� �ð�
};

// �� ���� �Ŵ��� Ŭ���� (�̱��� ����)
class UModelBufferManager
{
public:
    // �̱��� �ν��Ͻ� ������
    static UModelBufferManager* Get()
    {
        // �̱��� �ν��Ͻ�
        static UModelBufferManager* Instance = new UModelBufferManager();
        if (!Instance->bInitialized)
        {
            Instance->Initialize();
        }
        return Instance;
    }

    // ����̽� ����
    void SetDevice(ID3D11Device* InDevice) { Device = InDevice; }

    // �⺻ ������Ƽ�� �� �ؽ� ������
    size_t GetCubeHash() const { return CubeModelHash; }
    size_t GetSphereHash() const { return SphereModelHash; }
    size_t GetPlaneHash() const { return PlaneModelHash; }

    // �ʱ�ȭ �� ����
    bool Initialize();
    void Release();

    // ���� ���ҽ� ������
    FBufferResource* GetBufferByHash(size_t InHash);

    // ���� �����ͷκ��� �ؽ� ���� �Ǵ� ����
    size_t RegisterVertexData(const FVertexDataContainer& InVertexData);

    // �ؽ÷κ��� ���� ������ ����
    const FVertexDataContainer* GetVertexDataByHash(size_t InHash) const;

private:
    UModelBufferManager() = default;
    ~UModelBufferManager();

    bool bInitialized = false;

    // �⺻ ������Ƽ�� �� ���� �޼���
    void CreateDefaultPrimitives();
    FVertexDataContainer CreateCubeVertexData();
    FVertexDataContainer CreateSphereVertexData(int InSegments = 32);
    FVertexDataContainer CreatePlaneVertexData();

    // �ؽ� ��� ���� �޼���
    size_t CalculateHash(const FVertexDataContainer& InVertexData) const;

    //LRU �� �Լ� (Is A more Recently used than B)
    bool IsMoreRecentlyUsed(size_t tickA, size_t tickB);

    // ���� Ǯ ���� �޼���
    bool AddBufferToPool(size_t InHash, const FVertexDataContainer& InVertexData);
    void ReplaceBufferInPool();

    // ��� ����
    ID3D11Device* Device = nullptr;
    //Ŭ���� �ֱٿ� ���Ȱ�
    size_t CurrentTick = 0;

    // �⺻ ������Ƽ�� �� �ؽ�
    size_t CubeModelHash = 0;
    size_t SphereModelHash = 0;
    size_t PlaneModelHash = 0;

    // Ǯ ���� ���
    const size_t MAX_BUFFER_POOL_SIZE = 100;

    // ���� ������ �����̳� �� ���� Ǯ ��
    std::unordered_map<size_t, FVertexDataContainer> VertexDataMap;
    std::unordered_map<size_t, std::unique_ptr<FBufferResource>> BufferPool;

    // �⺻ �� �÷��� (�⺻ ���� LRU���� ����)
    std::unordered_map<size_t, bool> DefaultModelFlags;
};