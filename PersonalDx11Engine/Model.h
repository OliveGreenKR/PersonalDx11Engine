#pragma once
#include "Math.h"
#include "D3D.h"

using namespace DirectX;

/// <summary>
/// load model and make VertexBuffer?
/// </summary>
class FModel
{
public:
	//TODO : LoadModel by file, and make private VertexStuct
	struct FVertexSimple
	{
		XMFLOAT3 Position;    // Position
		XMFLOAT4 Color;       // Color
	};

public:
	FModel() = default;
	FModel(ID3D11Device* InDevice, const FVertexSimple* InVertices, const UINT InNumVertices);
	~FModel();

	void Release();

public:
	bool Initialize(ID3D11Device* InDevice, const FVertexSimple* InVertices, const UINT InNumVertices);

	__forceinline UINT GetNumVertices() const { return NumVertices; }
	ID3D11Buffer* GetVertexBuffer() { return VertexBuffer; }


public:
	const static FModel GetDefaultTriangle(ID3D11Device* InDevice);

private:
	bool CreateVertexBuffer(ID3D11Device* InDevice, const FVertexSimple* InVertices, const UINT InNumVertices);


private:
	bool bInitialized = false;
	UINT NumVertices;

	ID3D11Buffer* VertexBuffer;
};
