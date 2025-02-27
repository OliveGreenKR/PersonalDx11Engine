#pragma once
#include "Math.h"
#include "D3D.h"
#include <type_traits>
#include <memory>
#include <unordered_map>
#include <mutex>

using namespace DirectX;

struct VertexBufferInfo
{
	ID3D11Buffer* Buffer = nullptr;
	UINT NumVertices = 0;
	UINT Stride = 0;
};

/// <summary>
/// load model and make VertexBuffer?
/// </summary>
class UModel
{
public:

	struct FVertexData
	{
		XMFLOAT3 Position;
		XMFLOAT2 TexCoord;    
	};

	struct FVertexSimple 
	{
		XMFLOAT3 Position;    // Position
		XMFLOAT4 Color;       // Color
	};

private:
	// 버퍼의 고유 식별자 생성
	static size_t GenerateBufferHash(const void* vertices, size_t vertexCount, size_t stride) {
		return std::hash<const void*>{}(vertices) ^
			std::hash<size_t>{}(vertexCount) ^
			std::hash<size_t>{}(stride);
	}

public:
	UModel() = default;
	~UModel();

	void Release();

public:

	const static UModel GetSimpleTriangle(ID3D11Device* InDevice);
	static std::shared_ptr<UModel> GetDefaultTriangle(ID3D11Device* InDevice);
	static std::shared_ptr<UModel> GetDefaultCube(ID3D11Device* InDevice);
	static std::shared_ptr<UModel> GetDefaultSphere(ID3D11Device* InDevice);



public:
	template<typename T>
	const bool Initialize(ID3D11Device* InDevice, const T* InVertices, const size_t InNumVertices);
	
	__forceinline const VertexBufferInfo GetVertexBufferInfo() const { return VertexBufferInfo; }
	__forceinline const bool IsIntialized() const { return bIsInitialized; }

private:
	template<typename T>
	const bool CreateVertexBuffer(ID3D11Device* InDevice, const T* InVertices, const size_t InNumVertices);

private:
	bool bIsInitialized = false;
	VertexBufferInfo VertexBufferInfo;
};


/// <summary>
/// T : must be  the base of 'UModel::FVertexData'
/// </summary>
template<typename T>
inline const bool UModel::Initialize(ID3D11Device* InDevice, const T* InVertices, const size_t InNumVertices)
{
	bool result  = CreateVertexBuffer(InDevice, InVertices, InNumVertices);
	return bIsInitialized = result;
}

template<typename T>
inline const bool UModel::CreateVertexBuffer(ID3D11Device* InDevice, const T* InVertices, const size_t InNumVertices)
{
	if (!InDevice || !InVertices || InNumVertices == 0)
	{
		return false;
	}

	D3D11_BUFFER_DESC BufferDesc = {};
	BufferDesc.ByteWidth = static_cast<UINT>(InNumVertices * sizeof(T));
	BufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	BufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	//Buffer SubResource
	D3D11_SUBRESOURCE_DATA BufferSRD = {};
	BufferSRD.pSysMem = InVertices;

	//create buffer
	HRESULT result = InDevice->CreateBuffer(&BufferDesc, &BufferSRD, &VertexBufferInfo.Buffer);
	if (FAILED(result))
		return false;

	VertexBufferInfo.Stride = sizeof(T);
	VertexBufferInfo.NumVertices = InNumVertices;
	return true;
}
