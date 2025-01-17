#include "Model.h"



FModel::FModel(ID3D11Device* InDevice, const FVertexSimple* InVertices, const UINT InNumVertices)
{
	Initialize(InDevice, InVertices, InNumVertices);
}

FModel::~FModel()
{
	Shutdown();
}

bool FModel::Initialize(ID3D11Device* InDevice, const FVertexSimple* InVertices, const UINT InNumVertices)
{
	return CreateVertexBuffer(InDevice, InVertices, InNumVertices);
}


const FModel FModel::GetDefaultTriangle(ID3D11Device* InDevice)
{
	return FModel(InDevice,FModel::triangle_vertices,sizeof(triangle_vertices)/sizeof(FVertexSimple));
}

bool FModel::CreateVertexBuffer(ID3D11Device* InDevice, const FVertexSimple* InVertices, const UINT InNumVertices)
{
	if (!InDevice || !InVertices || InNumVertices == 0)
	{
		return false;
	}

	UINT ByteWidth = sizeof(FVertexSimple) * InNumVertices;
	//BufferDesc
	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.ByteWidth = ByteWidth;
	bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	//Buffer SubResource
	D3D11_SUBRESOURCE_DATA bufferSRD = { InVertices };
	//CreateBuffer
	HRESULT result = InDevice -> CreateBuffer(&bufferDesc, &bufferSRD, &VertexBuffer);
	if (FAILED(result))
		return false;

	NumVertices = InNumVertices;
	return bInitialized = true;
}

void FModel::Shutdown()
{
	if (VertexBuffer)
	{
		VertexBuffer->Release();
	}
}


