#pragma once
#include "Math.h"
#include "D3D.h"

using namespace DirectX;

/// <summary>
/// load model and make VertexBuffer
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

	const static FVertexSimple triangle_vertices[];

public:
	FModel() = default;
	FModel(ID3D11Device* InDevice, const FVertexSimple* InVertices, const UINT InNumVertices);
	~FModel();

private:
	

public:
	bool Initialize(ID3D11Device* InDevice, const FVertexSimple* InVertices, const UINT InNumVertices);

	__forceinline UINT GetIndexCount() const { return NumVertices; }
	const ID3D11Buffer* GetVertexBuffer() const { return VertexBuffer; }


public:
	const static FModel GetDefaultTriangle(ID3D11Device* InDevice);

private:
	bool CreateVertexBuffer(ID3D11Device* InDevice, const FVertexSimple* InVertices, const UINT InNumVertices);
	void Shutdown();

private:
	bool bInitialized = false;
	UINT NumVertices;

	ID3D11Buffer* VertexBuffer;
};


const FModel::FVertexSimple FModel::triangle_vertices[] =
{
	{ {  0.0f,  1.0f, 0.0f }, {  1.0f, 0.0f, 0.0f, 1.0f } }, // Top vertex (red)
	{ {  1.0f, -1.0f, 0.0f }, {  0.0f, 1.0f, 0.0f, 1.0f } }, // Bottom-right vertex (green)
	{ { -1.0f, -1.0f, 0.0f }, {  0.0f, 0.0f, 1.0f, 1.0f } }  // Bottom-left vertex (blue)
};
