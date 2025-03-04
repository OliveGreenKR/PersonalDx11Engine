#include "ModelBufferManager.h"
#include "Math.h"
#include <functional>

// FBufferResource �޼��� ����
FBufferResource::~FBufferResource()
{
    Release();
}

void FBufferResource::Release()
{
    if (VertexBuffer)
    {
        VertexBuffer->Release();
        VertexBuffer = nullptr;
    }

    if (IndexBuffer)
    {
        IndexBuffer->Release();
        IndexBuffer = nullptr;
    }

    VertexCount = 0;
    IndexCount = 0;
}

bool FBufferResource::Initialize(ID3D11Device* InDevice, const FVertexDataContainer& InVertexData)
{
    Release();

    if (!InDevice || InVertexData.Vertices.empty())
        return false;

    // ���� ���� ����
    D3D11_BUFFER_DESC vertexBufferDesc = {};
    vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    vertexBufferDesc.ByteWidth = static_cast<UINT>(sizeof(FVertexFormat) * InVertexData.Vertices.size());
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexBufferDesc.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA vertexData = {};
    vertexData.pSysMem = InVertexData.Vertices.data();

    HRESULT hr = InDevice->CreateBuffer(&vertexBufferDesc, &vertexData, &VertexBuffer);
    if (FAILED(hr))
        return false;

    // �ε����� �ִ� ��� �ε��� ���� ����
    if (!InVertexData.Indices.empty())
    {
        D3D11_BUFFER_DESC indexBufferDesc = {};
        indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
        indexBufferDesc.ByteWidth = static_cast<UINT>(sizeof(UINT) * InVertexData.Indices.size());
        indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        indexBufferDesc.CPUAccessFlags = 0;

        D3D11_SUBRESOURCE_DATA indexData = {};
        indexData.pSysMem = InVertexData.Indices.data();

        hr = InDevice->CreateBuffer(&indexBufferDesc, &indexData, &IndexBuffer);
        if (FAILED(hr))
        {
            VertexBuffer->Release();
            VertexBuffer = nullptr;
            return false;
        }

        IndexCount = static_cast<UINT>(InVertexData.Indices.size());
    }

    VertexCount = static_cast<UINT>(InVertexData.Vertices.size());
    Stride = sizeof(FVertexFormat);
    Offset = 0;

    return true;
}

UModelBufferManager::~UModelBufferManager()
{
    Release();
}

bool UModelBufferManager::Initialize()
{
    if (!Device)
        return false;

    // �⺻ ������Ƽ�� �� ����
    CreateDefaultPrimitives();
    bInitialized = true;
    return true;
}

void UModelBufferManager::Release()
{
    BufferPool.clear();
    VertexDataMap.clear();
    DefaultModelFlags.clear();
}

void UModelBufferManager::CreateDefaultPrimitives()
{
    // ť�� ���� ������ ���� �� ���
    FVertexDataContainer cubeData = CreateCubeVertexData();
    CubeModelHash = RegisterVertexData(cubeData);
    DefaultModelFlags[CubeModelHash] = true;

    // �� ���� ������ ���� �� ���
    FVertexDataContainer sphereData = CreateSphereVertexData();
    SphereModelHash = RegisterVertexData(sphereData);
    DefaultModelFlags[SphereModelHash] = true;

    // ��� ���� ������ ���� �� ���
    FVertexDataContainer planeData = CreatePlaneVertexData();
    PlaneModelHash = RegisterVertexData(planeData);
    DefaultModelFlags[PlaneModelHash] = true;
}

FVertexDataContainer UModelBufferManager::CreateCubeVertexData()
{
    FVertexDataContainer data;

    // ť�� ���� ������ (��ġ�� �ؽ�ó ��ǥ�� ����)
    float size = 0.5f;

    // 24���� ���� (�麰�� �ٸ� ����, �ؽ�ó ��ǥ�� ����)
    // ���� (Z+)
    data.Vertices.push_back({ {-size, -size, size}, {0.0f, 1.0f} });  // ���ϴ�
    data.Vertices.push_back({ {size, -size, size}, {1.0f, 1.0f} });   // ���ϴ�
    data.Vertices.push_back({ {size, size, size}, {1.0f, 0.0f} });    // ����
    data.Vertices.push_back({ {-size, size, size}, {0.0f, 0.0f} });   // �»��

    // �ĸ� (Z-)
    data.Vertices.push_back({ {size, -size, -size}, {0.0f, 1.0f} });  // ���ϴ�
    data.Vertices.push_back({ {-size, -size, -size}, {1.0f, 1.0f} }); // ���ϴ�
    data.Vertices.push_back({ {-size, size, -size}, {1.0f, 0.0f} });  // ����
    data.Vertices.push_back({ {size, size, -size}, {0.0f, 0.0f} });   // �»��

    // ������ (X+)
    data.Vertices.push_back({ {size, -size, size}, {0.0f, 1.0f} });   // ���ϴ�
    data.Vertices.push_back({ {size, -size, -size}, {1.0f, 1.0f} });  // ���ϴ�
    data.Vertices.push_back({ {size, size, -size}, {1.0f, 0.0f} });   // ����
    data.Vertices.push_back({ {size, size, size}, {0.0f, 0.0f} });    // �»��

    // ������ (X-)
    data.Vertices.push_back({ {-size, -size, -size}, {0.0f, 1.0f} }); // ���ϴ�
    data.Vertices.push_back({ {-size, -size, size}, {1.0f, 1.0f} });  // ���ϴ�
    data.Vertices.push_back({ {-size, size, size}, {1.0f, 0.0f} });   // ����
    data.Vertices.push_back({ {-size, size, -size}, {0.0f, 0.0f} });  // �»��

    // ��� (Y+)
    data.Vertices.push_back({ {-size, size, size}, {0.0f, 1.0f} });   // ���ϴ�
    data.Vertices.push_back({ {size, size, size}, {1.0f, 1.0f} });    // ���ϴ�
    data.Vertices.push_back({ {size, size, -size}, {1.0f, 0.0f} });   // ����
    data.Vertices.push_back({ {-size, size, -size}, {0.0f, 0.0f} });  // �»��

    // �ϸ� (Y-)
    data.Vertices.push_back({ {-size, -size, -size}, {0.0f, 1.0f} }); // ���ϴ�
    data.Vertices.push_back({ {size, -size, -size}, {1.0f, 1.0f} });  // ���ϴ�
    data.Vertices.push_back({ {size, -size, size}, {1.0f, 0.0f} });   // ����
    data.Vertices.push_back({ {-size, -size, size}, {0.0f, 0.0f} });  // �»��

    // 12�� �ﰢ�� (6�� * 2 �ﰢ��)�� �ε���
    data.Indices = {
        // ���� (Z+)
        0, 1, 2,
        0, 2, 3,

        // �ĸ� (Z-)
        4, 5, 6,
        4, 6, 7,

        // ������ (X+)
        8, 9, 10,
        8, 10, 11,

        // ������ (X-)
        12, 13, 14,
        12, 14, 15,

        // ��� (Y+)
        16, 17, 18,
        16, 18, 19,

        // �ϸ� (Y-)
        20, 21, 22,
        20, 22, 23
    };

    return data;
}

FVertexDataContainer UModelBufferManager::CreateSphereVertexData(int InSegments)
{
    FVertexDataContainer data;

    // �� ���� ������ ����
    const float radius = 0.5f;
    const int segments = InSegments;
    const int rings = segments;  // �� �ε巯�� ���� ���� �� �� ����

    // UV �ؽ�ó ������ ���� ���� ���� (���� ����)
    for (int ring = 0; ring <= rings; ++ring)
    {
        // ������ (0 = �ϱ�, �� = ����)
        float phi = static_cast<float>(PI) * static_cast<float>(ring) / static_cast<float>(rings);
        float v = static_cast<float>(ring) / static_cast<float>(rings); // v �ؽ�ó ��ǥ (0 ~ 1)

        for (int segment = 0; segment <= segments; ++segment)
        {
            // �浵�� (0 ~ 2��)
            float theta = static_cast<float>(2.0 * PI) * static_cast<float>(segment) / static_cast<float>(segments);
            float u = static_cast<float>(segment) / static_cast<float>(segments); // u �ؽ�ó ��ǥ (0 ~ 1)

            // ���� ��ǥ�� ��ī��Ʈ ��ǥ�� ��ȯ
            float x = radius * sin(phi) * cos(theta);
            float y = radius * cos(phi);
            float z = radius * sin(phi) * sin(theta);

            data.Vertices.push_back({ {x, y, z}, {u, v} });
        }
    }

    // �ε��� ���� (�� ������ ����)
    for (int ring = 0; ring < rings; ++ring)
    {
        for (int segment = 0; segment < segments; ++segment)
        {
            // ���� ���� ���� ���� ���� �ε��� ���
            int currentRingStart = ring * (segments + 1);
            int nextRingStart = (ring + 1) * (segments + 1);

            int currentVertex = currentRingStart + segment;
            int nextVertex = currentRingStart + segment + 1;
            int currentVertexNextRing = nextRingStart + segment;
            int nextVertexNextRing = nextRingStart + segment + 1;

            // �� �ﰢ�� ���� (�簢��)
            data.Indices.push_back(currentVertex);
            data.Indices.push_back(nextVertex);
            data.Indices.push_back(currentVertexNextRing);

            data.Indices.push_back(nextVertex);
            data.Indices.push_back(nextVertexNextRing);
            data.Indices.push_back(currentVertexNextRing);
        }
    }

    return data;
}

FVertexDataContainer UModelBufferManager::CreatePlaneVertexData()
{
    FVertexDataContainer data;
    
    // ������ ��� ���� ������ ���� (XZ ���)
    float size = 0.5f;
    
    // 4�� ���� (���簢��)
    data.Vertices = {
        { {-size, 0.0f, -size}, {0.0f, 1.0f} }, // ���ϴ�
        { {size, 0.0f, -size}, {1.0f, 1.0f} },  // ���ϴ�
        { {size, 0.0f, size}, {1.0f, 0.0f} },   // ����
        { {-size, 0.0f, size}, {0.0f, 0.0f} }   // �»��
    };
    
    // 2�� �ﰢ���� �ε���
    data.Indices = {
        0, 1, 2,  // ù ��° �ﰢ��
        0, 2, 3   // �� ��° �ﰢ��
    };
    
    return data;
}

size_t UModelBufferManager::CalculateHash(const FVertexDataContainer& InVertexData) const
{
    // �ؽ� ����� ���� ������ ����
    std::size_t hash = 0;

    // ���� �������� ��� ��Ҹ� �ؽÿ� ����
    for (const auto& vertex : InVertexData.Vertices)
    {
        // ��ġ �ؽ�
        hash ^= std::hash<float>{}(vertex.Position.x) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        hash ^= std::hash<float>{}(vertex.Position.y) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        hash ^= std::hash<float>{}(vertex.Position.z) + 0x9e3779b9 + (hash << 6) + (hash >> 2);

        // ���� �ؽ�
        hash ^= std::hash<float>{}(vertex.TexCoord.x) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        hash ^= std::hash<float>{}(vertex.TexCoord.y) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    }

    // �ε��� ������ �ؽ� �߰�
    for (const auto& index : InVertexData.Indices)
    {
        hash ^= std::hash<UINT>{}(index)+0x9e3779b9 + (hash << 6) + (hash >> 2);
    }

    return hash;
}

bool UModelBufferManager::IsMoreRecentlyUsed(size_t tickA, size_t tickB)
{
    return (tickA - tickB) < (size_t(-1) / 2);
}

size_t UModelBufferManager::RegisterVertexData(const FVertexDataContainer& InVertexData)
{
    // �ؽ� ���
    size_t hash = CalculateHash(InVertexData);

    // �̹� ��ϵ� ���� ���������� Ȯ��
    if (VertexDataMap.find(hash) == VertexDataMap.end())
    {
        // ���ο� ���� ������ ���
        VertexDataMap[hash] = InVertexData;

        // ���� Ǯ�� �߰�
        AddBufferToPool(hash, InVertexData);
    }

    return hash;
}

const FVertexDataContainer* UModelBufferManager::GetVertexDataByHash(size_t InHash) const
{
    // �ؽÿ� �ش��ϴ� ���� ������ ��ȯ
    auto it = VertexDataMap.find(InHash);
    if (it != VertexDataMap.end())
    {
        return &it->second;
    }

    return nullptr;
}

FBufferResource* UModelBufferManager::GetBufferByHash(size_t InHash)
{
    // ��û�� �ؽÿ� �ش��ϴ� ���� ���ҽ� ã��
    auto it = BufferPool.find(InHash);
    if (it != BufferPool.end())
    {
        // ���� �ð� ������Ʈ (LRU �˰����� ����)
        it->second->UpdateAccessTick(++CurrentTick);
        return it->second.get();
    }

    return nullptr;
}

bool UModelBufferManager::AddBufferToPool(size_t InHash, const FVertexDataContainer& InVertexData)
{
    // Ǯ�� �̹� �ִ� ũ������ Ȯ��
    if (BufferPool.size() >= MAX_BUFFER_POOL_SIZE)
    {
        // Ǯ�� �� á���� LRU ������� ���� ��ü
        ReplaceBufferInPool();
    }

    // ���ο� ���� ���ҽ� ����
    auto newBuffer = std::make_unique<FBufferResource>();
    if (!newBuffer->Initialize(Device, InVertexData))
    {
        return false;
    }

    // ���� ���� �ð� ����
    newBuffer->UpdateAccessTick(++CurrentTick);

    // Ǯ�� ���� �߰�
    BufferPool[InHash] = std::move(newBuffer);

    return true;
}

void UModelBufferManager::ReplaceBufferInPool()
{
    // LRU �˰����� ����Ͽ� ���� ���� ������ ���� ���� ã��
    size_t lruHash = 0;
    size_t oldestTick = SIZE_MAX;

    for (const auto& pair : BufferPool)
    {
        // �⺻ ���� ��ü���� ����
        if (DefaultModelFlags.find(pair.first) != DefaultModelFlags.end() && DefaultModelFlags[pair.first])
        {
            continue;
        }

        if (!IsMoreRecentlyUsed(pair.second->GetLastAccessTick(), oldestTick))
        {
            oldestTick = pair.second->GetLastAccessTick();
            lruHash = pair.first;
        }
    }

    // ��ü ������ ���۸� ã�Ҵ��� Ȯ��
    if (lruHash != 0)
    {
        // ���� ���� ������ ���� ���� ����
        BufferPool.erase(lruHash);
    }
}