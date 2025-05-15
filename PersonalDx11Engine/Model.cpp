#include "Model.h"
#include "D3DShader.h"
#include "ResourceHandle.h"
#include "ResourceManager.h"
#include "define.h"


//bool UModel::InitializeAsCube() {
//    auto manager = UModel::Get();
//    DataHash = manager->GetCubeHash();
//    bIsInitialized = (DataHash != 0);
//    return bIsInitialized;
//}

bool UModel::CreateBuffers(ID3D11Device* InDevice, const FVertexDataContainer& InVertexData)
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

UModel::~UModel()
{
    Release();
}

void UModel::Release()
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
    Stride = 0;
    Offset = 0;
}


bool UModel::Load(IRenderHardware* RenderHardware, const std::wstring& Path)
{
    if (!RenderHardware || !RenderHardware->GetDevice())
    {
        return false;
    }

    //TODO
    // readFile to FVertexDataContaienr
    // Initialize with Device
    if (Path == MDL_CUBE)
    {
        // 큐브 정점 데이터 생성 및 등록
        FVertexDataContainer cubeData = UModel::CreateCubeVertexData();
         return CreateBuffers(RenderHardware->GetDevice(), cubeData);
    }
    else if (Path == MDL_SPHERE_Mid)
    {
        // 구 정점 데이터 생성 및 등록
        FVertexDataContainer sphereData = UModel::CreateSphereVertexData(12);
        return CreateBuffers(RenderHardware->GetDevice(), sphereData);
        
    }
	else if (Path == MDL_SPHERE_Low)
	{
		// 구 정점 데이터 생성 및 등록
		FVertexDataContainer sphereData = UModel::CreateSphereVertexData(6);
		return CreateBuffers(RenderHardware->GetDevice(), sphereData);

	}
	else if (Path == MDL_SPHERE_High)
	{
		// 구 정점 데이터 생성 및 등록
		FVertexDataContainer sphereData = UModel::CreateSphereVertexData(24);
		return CreateBuffers(RenderHardware->GetDevice(), sphereData);

	}
    else if (Path == MDL_PLANE)
    {
        // 평면 정점 데이터 생성 및 등록
        FVertexDataContainer planeData = UModel::CreatePlaneVertexData();
        return CreateBuffers(RenderHardware->GetDevice(), planeData);
    }

    return false;
}

bool UModel::LoadAsync(IRenderHardware* RenderHardware, const std::wstring& Path)
{
    //TODO
    return Load(RenderHardware, Path);
}

size_t UModel::GetMemorySize() const
{
    //TODO
    //근사
    size_t size = VertexCount * sizeof(FVertexDataContainer) + IndexCount * sizeof(UINT);
    return size;
}

/////////////////////
/////           /////
/////////////////////

FVertexDataContainer UModel::CreateCubeVertexData()
{
	FVertexDataContainer data;

	// 큐브 정점 데이터 (위치와 텍스처 좌표만 포함)
	float size = 0.5f;

	const Vector3 frontNormal = Vector3(0.0f, 0.0f, 1.0f);   // 전면 (Z+)
	const Vector3 backNormal = Vector3(0.0f, 0.0f, -1.0f);   // 후면 (Z-)
	const Vector3 rightNormal = Vector3(1.0f, 0.0f, 0.0f);   // 우측면 (X+)
	const Vector3 leftNormal = Vector3(-1.0f, 0.0f, 0.0f);   // 좌측면 (X-)
	const Vector3 topNormal = Vector3(0.0f, 1.0f, 0.0f);     // 상면 (Y+)
	const Vector3 bottomNormal = Vector3(0.0f, -1.0f, 0.0f); // 하면 (Y-)

	// 24개의 정점 (면별로 다른 정점, 텍스처 좌표를 위해)
	// 전면 (Z+)
	data.Vertices.push_back({ {-size, -size, size}, {0.0f, 1.0f}, frontNormal });  // 좌하단
	data.Vertices.push_back({ {size, -size, size}, {1.0f, 1.0f}, frontNormal });   // 우하단
	data.Vertices.push_back({ {size, size, size}, {1.0f, 0.0f}, frontNormal });    // 우상단
	data.Vertices.push_back({ {-size, size, size}, {0.0f, 0.0f}, frontNormal });   // 좌상단

	// 후면 (Z-)
	data.Vertices.push_back({ {size, -size, -size}, {0.0f, 1.0f}, backNormal });  // 좌하단
	data.Vertices.push_back({ {-size, -size, -size}, {1.0f, 1.0f}, backNormal }); // 우하단
	data.Vertices.push_back({ {-size, size, -size}, {1.0f, 0.0f}, backNormal });  // 우상단
	data.Vertices.push_back({ {size, size, -size}, {0.0f, 0.0f}, backNormal });   // 좌상단

	// 우측면 (X+)
	data.Vertices.push_back({ {size, -size, size}, {0.0f, 1.0f}, rightNormal });   // 좌하단
	data.Vertices.push_back({ {size, -size, -size}, {1.0f, 1.0f}, rightNormal });  // 우하단
	data.Vertices.push_back({ {size, size, -size}, {1.0f, 0.0f}, rightNormal });   // 우상단
	data.Vertices.push_back({ {size, size, size}, {0.0f, 0.0f}, rightNormal });    // 좌상단

	// 좌측면 (X-)
	data.Vertices.push_back({ {-size, -size, -size}, {0.0f, 1.0f}, leftNormal }); // 좌하단
	data.Vertices.push_back({ {-size, -size, size}, {1.0f, 1.0f}, leftNormal });  // 우하단
	data.Vertices.push_back({ {-size, size, size}, {1.0f, 0.0f}, leftNormal });   // 우상단
	data.Vertices.push_back({ {-size, size, -size}, {0.0f, 0.0f}, leftNormal });  // 좌상단

	// 상면 (Y+)
	data.Vertices.push_back({ {-size, size, size}, {0.0f, 1.0f}, topNormal });   // 좌하단
	data.Vertices.push_back({ {size, size, size}, {1.0f, 1.0f}, topNormal });    // 우하단
	data.Vertices.push_back({ {size, size, -size}, {1.0f, 0.0f}, topNormal });   // 우상단
	data.Vertices.push_back({ {-size, size, -size}, {0.0f, 0.0f}, topNormal });  // 좌상단

	// 하면 (Y-)
	data.Vertices.push_back({ {-size, -size, -size}, {0.0f, 1.0f}, bottomNormal }); // 좌하단
	data.Vertices.push_back({ {size, -size, -size}, {1.0f, 1.0f}, bottomNormal });  // 우하단
	data.Vertices.push_back({ {size, -size, size}, {1.0f, 0.0f}, bottomNormal });   // 우상단
	data.Vertices.push_back({ {-size, -size, size}, {0.0f, 0.0f}, bottomNormal });  // 좌상단

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

	return data;
}
FVertexDataContainer UModel::CreateSphereVertexData(int InSegments)
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

			// 노멀은 위치를 정규화한 벡터 (구의 중심에서 표면으로의 방향)
			Vector3 normal = Vector3(x, y, z).GetNormalized(); 
			//position, text, normal
			data.Vertices.push_back({ {x, y, z}, {u, v}, normal });
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

FVertexDataContainer UModel::CreatePlaneVertexData()
{
	FVertexDataContainer data;

	// 간단한 평면 정점 데이터 생성 (XZ 평면)
	float size = 0.5f;

	// 평면의 노멀 (Y+)
	const Vector3 planeNormal = Vector3(0.0f, 1.0f, 0.0f);

	// 4개 정점 (정사각형)
	data.Vertices = {
		{ {-size, 0.0f, -size}, {0.0f, 1.0f}, planeNormal }, // 좌하단
		{ {size, 0.0f, -size}, {1.0f, 1.0f}, planeNormal },  // 우하단
		{ {size, 0.0f, size}, {1.0f, 0.0f}, planeNormal },   // 우상단
		{ {-size, 0.0f, size}, {0.0f, 0.0f}, planeNormal }   // 좌상단
	};

	// 2개 삼각형의 인덱스
	data.Indices = {
		0, 1, 2,  // 첫 번째 삼각형
		0, 2, 3   // 두 번째 삼각형
	};

	return data;
}
