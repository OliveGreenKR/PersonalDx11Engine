#include "Model.h"



//UModel::UModel(ID3D11Device* InDevice, const FVertexSimple* InVertices, const UINT InNumVertices)
//{
//	Initialize(InDevice, InVertices, InNumVertices);
//}

UModel::~UModel()
{
	Release();
}

const UModel UModel::GetDefaultTriangle(ID3D11Device* InDevice)
{
	const static FVertexSimple triangle_vertices[] = {
	{ {  0.0f,  1.0f, 0.0f }, {  1.0f, 0.0f, 0.0f, 1.0f } }, // Top vertex (red)
	{ {  1.0f, -1.0f, 0.0f }, {  0.0f, 1.0f, 0.0f, 1.0f } }, // Bottom-right vertex (green)
	{ { -1.0f, -1.0f, 0.0f }, {  0.0f, 0.0f, 1.0f, 1.0f } }  // Bottom-left vertex (blue)
	};
	UModel model;
	model.Initialize<FVertexSimple>(InDevice, triangle_vertices, sizeof(triangle_vertices) / sizeof(FVertexSimple));
	return model;
}

void UModel::Release()
{
	if (VertexBufferInfo.Buffer)
	{
		VertexBufferInfo.Buffer->Release();
	}
}


