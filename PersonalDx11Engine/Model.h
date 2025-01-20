#pragma once
#include "Math.h"
#include "D3D.h"

using namespace DirectX;

/// <summary>
/// load model and make VertexBuffer?
/// </summary>
class UModel
{
public:
	//TODO : LoadModel by file, and make private VertexStuct
	struct FVertexSimple
	{
		XMFLOAT3 Position;    // Position
		XMFLOAT4 Color;       // Color
	};

	struct FStaticVertexData
	{
		DirectX::XMFLOAT3 Position;    // 12 bytes
		//DirectX::XMFLOAT3 Normal;      // 12 bytes
		//DirectX::XMFLOAT2 TexCoord;    // 8 bytes
	};

	struct FDynamicVertexData
	{
		DirectX::XMFLOAT4 Color;       // 16 bytes
		DirectX::XMFLOAT4 BoneWeights; // 16 bytes
		UINT BoneIndices[4];           // 16 bytes
	};

public:
	UModel() = default;
	UModel(ID3D11Device* InDevice, const FVertexSimple* InVertices, const UINT InNumVertices);
	~UModel();

	void Release();

public:
	bool Initialize(ID3D11Device* InDevice, const FVertexSimple* InVertices, const UINT InNumVertices);

	__forceinline UINT GetNumVertices() const { return NumVertices; }
	ID3D11Buffer* GetVertexBuffer() { return VertexBuffer; }
	UINT GetVertexStride() { return sizeof(FVertexSimple); }

	const bool IsIntialized() { return bInitialized; }

public:
	const static UModel GetDefaultTriangle(ID3D11Device* InDevice);

private:
	bool CreateVertexBuffer(ID3D11Device* InDevice, const FVertexSimple* InVertices, const UINT InNumVertices);


private:
	bool bInitialized = false;
	UINT NumVertices;

	ID3D11Buffer* VertexBuffer;
};
