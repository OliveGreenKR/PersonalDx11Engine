#include "Model.h"



UModel::UModel(ID3D11Device* InDevice, const FVertexSimple* InVertices, const UINT InNumVertices)
{
	Initialize(InDevice, InVertices, InNumVertices);
}

UModel::~UModel()
{
	Release();
}

bool UModel::Initialize(ID3D11Device* InDevice, const FVertexSimple* InVertices, const UINT InNumVertices)
{
	return CreateVertexBuffer(InDevice, InVertices, InNumVertices);
}


const UModel UModel::GetDefaultTriangle(ID3D11Device* InDevice)
{
	const static FVertexSimple triangle_vertices[] = {
	{ {  0.0f,  1.0f, 0.0f }, {  1.0f, 0.0f, 0.0f, 1.0f } }, // Top vertex (red)
	{ {  1.0f, -1.0f, 0.0f }, {  0.0f, 1.0f, 0.0f, 1.0f } }, // Bottom-right vertex (green)
	{ { -1.0f, -1.0f, 0.0f }, {  0.0f, 0.0f, 1.0f, 1.0f } }  // Bottom-left vertex (blue)
	};
	return UModel(InDevice,triangle_vertices,sizeof(triangle_vertices)/sizeof(FVertexSimple));
}

bool UModel::CreateVertexBuffer(ID3D11Device* InDevice, const FVertexSimple* InVertices, const UINT InNumVertices)
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

void UModel::Release()
{
	if (VertexBuffer)
	{
		VertexBuffer->Release();
	}
}


