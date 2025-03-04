#include "ModelBufferManager.h"
#include "Math.h"
#include <functional>

// FBufferResource 메서드 구현
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

    // 정점 버퍼 생성
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

    // 인덱스가 있는 경우 인덱스 버퍼 생성
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

    // 기본 프리미티브 모델 생성
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
    // 큐브 정점 데이터 생성 및 등록
    FVertexDataContainer cubeData = CreateCubeVertexData();
    CubeModelHash = RegisterVertexData(cubeData);
    DefaultModelFlags[CubeModelHash] = true;

    // 구 정점 데이터 생성 및 등록
    FVertexDataContainer sphereData = CreateSphereVertexData();
    SphereModelHash = RegisterVertexData(sphereData);
    DefaultModelFlags[SphereModelHash] = true;

    // 평면 정점 데이터 생성 및 등록
    FVertexDataContainer planeData = CreatePlaneVertexData();
    PlaneModelHash = RegisterVertexData(planeData);
    DefaultModelFlags[PlaneModelHash] = true;
}

FVertexDataContainer UModelBufferManager::CreateCubeVertexData()
{
    FVertexDataContainer data;

    // 큐브 정점 데이터 (위치와 텍스처 좌표만 포함)
    float size = 0.5f;

    // 24개의 정점 (면별로 다른 정점, 텍스처 좌표를 위해)
    // 전면 (Z+)
    data.Vertices.push_back({ {-size, -size, size}, {0.0f, 1.0f} });  // 좌하단
    data.Vertices.push_back({ {size, -size, size}, {1.0f, 1.0f} });   // 우하단
    data.Vertices.push_back({ {size, size, size}, {1.0f, 0.0f} });    // 우상단
    data.Vertices.push_back({ {-size, size, size}, {0.0f, 0.0f} });   // 좌상단

    // 후면 (Z-)
    data.Vertices.push_back({ {size, -size, -size}, {0.0f, 1.0f} });  // 좌하단
    data.Vertices.push_back({ {-size, -size, -size}, {1.0f, 1.0f} }); // 우하단
    data.Vertices.push_back({ {-size, size, -size}, {1.0f, 0.0f} });  // 우상단
    data.Vertices.push_back({ {size, size, -size}, {0.0f, 0.0f} });   // 좌상단

    // 우측면 (X+)
    data.Vertices.push_back({ {size, -size, size}, {0.0f, 1.0f} });   // 좌하단
    data.Vertices.push_back({ {size, -size, -size}, {1.0f, 1.0f} });  // 우하단
    data.Vertices.push_back({ {size, size, -size}, {1.0f, 0.0f} });   // 우상단
    data.Vertices.push_back({ {size, size, size}, {0.0f, 0.0f} });    // 좌상단

    // 좌측면 (X-)
    data.Vertices.push_back({ {-size, -size, -size}, {0.0f, 1.0f} }); // 좌하단
    data.Vertices.push_back({ {-size, -size, size}, {1.0f, 1.0f} });  // 우하단
    data.Vertices.push_back({ {-size, size, size}, {1.0f, 0.0f} });   // 우상단
    data.Vertices.push_back({ {-size, size, -size}, {0.0f, 0.0f} });  // 좌상단

    // 상면 (Y+)
    data.Vertices.push_back({ {-size, size, size}, {0.0f, 1.0f} });   // 좌하단
    data.Vertices.push_back({ {size, size, size}, {1.0f, 1.0f} });    // 우하단
    data.Vertices.push_back({ {size, size, -size}, {1.0f, 0.0f} });   // 우상단
    data.Vertices.push_back({ {-size, size, -size}, {0.0f, 0.0f} });  // 좌상단

    // 하면 (Y-)
    data.Vertices.push_back({ {-size, -size, -size}, {0.0f, 1.0f} }); // 좌하단
    data.Vertices.push_back({ {size, -size, -size}, {1.0f, 1.0f} });  // 우하단
    data.Vertices.push_back({ {size, -size, size}, {1.0f, 0.0f} });   // 우상단
    data.Vertices.push_back({ {-size, -size, size}, {0.0f, 0.0f} });  // 좌상단

    // 12개 삼각형 (6면 * 2 삼각형)의 인덱스
    data.Indices = {
        // 전면 (Z+)
        0, 1, 2,
        0, 2, 3,

        // 후면 (Z-)
        4, 5, 6,
        4, 6, 7,

        // 우측면 (X+)
        8, 9, 10,
        8, 10, 11,

        // 좌측면 (X-)
        12, 13, 14,
        12, 14, 15,

        // 상면 (Y+)
        16, 17, 18,
        16, 18, 19,

        // 하면 (Y-)
        20, 21, 22,
        20, 22, 23
    };

    return data;
}

FVertexDataContainer UModelBufferManager::CreateSphereVertexData(int InSegments)
{
    FVertexDataContainer data;

    // 구 정점 데이터 생성
    const float radius = 0.5f;
    const int segments = InSegments;
    const int rings = segments;  // 더 부드러운 모델을 위해 링 수 증가

    // UV 텍스처 매핑을 위한 정점 생성 (극점 포함)
    for (int ring = 0; ring <= rings; ++ring)
    {
        // 위도각 (0 = 북극, π = 남극)
        float phi = static_cast<float>(PI) * static_cast<float>(ring) / static_cast<float>(rings);
        float v = static_cast<float>(ring) / static_cast<float>(rings); // v 텍스처 좌표 (0 ~ 1)

        for (int segment = 0; segment <= segments; ++segment)
        {
            // 경도각 (0 ~ 2π)
            float theta = static_cast<float>(2.0 * PI) * static_cast<float>(segment) / static_cast<float>(segments);
            float u = static_cast<float>(segment) / static_cast<float>(segments); // u 텍스처 좌표 (0 ~ 1)

            // 구면 좌표를 데카르트 좌표로 변환
            float x = radius * sin(phi) * cos(theta);
            float y = radius * cos(phi);
            float z = radius * sin(phi) * sin(theta);

            data.Vertices.push_back({ {x, y, z}, {u, v} });
        }
    }

    // 인덱스 생성 (띠 구조로 연결)
    for (int ring = 0; ring < rings; ++ring)
    {
        for (int segment = 0; segment < segments; ++segment)
        {
            // 현재 링과 다음 링의 정점 인덱스 계산
            int currentRingStart = ring * (segments + 1);
            int nextRingStart = (ring + 1) * (segments + 1);

            int currentVertex = currentRingStart + segment;
            int nextVertex = currentRingStart + segment + 1;
            int currentVertexNextRing = nextRingStart + segment;
            int nextVertexNextRing = nextRingStart + segment + 1;

            // 두 삼각형 생성 (사각형)
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
    
    // 간단한 평면 정점 데이터 생성 (XZ 평면)
    float size = 0.5f;
    
    // 4개 정점 (정사각형)
    data.Vertices = {
        { {-size, 0.0f, -size}, {0.0f, 1.0f} }, // 좌하단
        { {size, 0.0f, -size}, {1.0f, 1.0f} },  // 우하단
        { {size, 0.0f, size}, {1.0f, 0.0f} },   // 우상단
        { {-size, 0.0f, size}, {0.0f, 0.0f} }   // 좌상단
    };
    
    // 2개 삼각형의 인덱스
    data.Indices = {
        0, 1, 2,  // 첫 번째 삼각형
        0, 2, 3   // 두 번째 삼각형
    };
    
    return data;
}

size_t UModelBufferManager::CalculateHash(const FVertexDataContainer& InVertexData) const
{
    // 해시 계산을 위한 간단한 구현
    std::size_t hash = 0;

    // 정점 데이터의 모든 요소를 해시에 결합
    for (const auto& vertex : InVertexData.Vertices)
    {
        // 위치 해시
        hash ^= std::hash<float>{}(vertex.Position.x) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        hash ^= std::hash<float>{}(vertex.Position.y) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        hash ^= std::hash<float>{}(vertex.Position.z) + 0x9e3779b9 + (hash << 6) + (hash >> 2);

        // 색상 해시
        hash ^= std::hash<float>{}(vertex.TexCoord.x) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        hash ^= std::hash<float>{}(vertex.TexCoord.y) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    }

    // 인덱스 데이터 해시 추가
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
    // 해시 계산
    size_t hash = CalculateHash(InVertexData);

    // 이미 등록된 정점 데이터인지 확인
    if (VertexDataMap.find(hash) == VertexDataMap.end())
    {
        // 새로운 정점 데이터 등록
        VertexDataMap[hash] = InVertexData;

        // 버퍼 풀에 추가
        AddBufferToPool(hash, InVertexData);
    }

    return hash;
}

const FVertexDataContainer* UModelBufferManager::GetVertexDataByHash(size_t InHash) const
{
    // 해시에 해당하는 정점 데이터 반환
    auto it = VertexDataMap.find(InHash);
    if (it != VertexDataMap.end())
    {
        return &it->second;
    }

    return nullptr;
}

FBufferResource* UModelBufferManager::GetBufferByHash(size_t InHash)
{
    // 요청된 해시에 해당하는 버퍼 리소스 찾기
    auto it = BufferPool.find(InHash);
    if (it != BufferPool.end())
    {
        // 접근 시간 업데이트 (LRU 알고리즘을 위해)
        it->second->UpdateAccessTick(++CurrentTick);
        return it->second.get();
    }

    return nullptr;
}

bool UModelBufferManager::AddBufferToPool(size_t InHash, const FVertexDataContainer& InVertexData)
{
    // 풀이 이미 최대 크기인지 확인
    if (BufferPool.size() >= MAX_BUFFER_POOL_SIZE)
    {
        // 풀이 꽉 찼으면 LRU 기반으로 버퍼 교체
        ReplaceBufferInPool();
    }

    // 새로운 버퍼 리소스 생성
    auto newBuffer = std::make_unique<FBufferResource>();
    if (!newBuffer->Initialize(Device, InVertexData))
    {
        return false;
    }

    // 현재 접근 시간 설정
    newBuffer->UpdateAccessTick(++CurrentTick);

    // 풀에 버퍼 추가
    BufferPool[InHash] = std::move(newBuffer);

    return true;
}

void UModelBufferManager::ReplaceBufferInPool()
{
    // LRU 알고리즘을 사용하여 가장 오래 사용되지 않은 버퍼 찾기
    size_t lruHash = 0;
    size_t oldestTick = SIZE_MAX;

    for (const auto& pair : BufferPool)
    {
        // 기본 모델은 교체하지 않음
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

    // 교체 가능한 버퍼를 찾았는지 확인
    if (lruHash != 0)
    {
        // 가장 오래 사용되지 않은 버퍼 제거
        BufferPool.erase(lruHash);
    }
}