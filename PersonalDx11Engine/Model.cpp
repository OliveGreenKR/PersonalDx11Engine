#include "Model.h"

UModel::~UModel()
{
	Release();
}

const UModel UModel::GetSimpleTriangle(ID3D11Device* InDevice)
{
	const static FVertexSimple triangle_vertices[] = {
	{ {  0.0f,  1.0f, 0.0f }, {  1.0f, 0.0f, 0.0f, 1.0f } }, // Top vertex (red)
	{ {  1.0f, -1.0f, 0.0f }, {  0.0f, 1.0f, 0.0f, 1.0f } }, // Bottom-right vertex (green)
	{ { -1.0f, -1.0f, 0.0f }, {  0.0f, 0.0f, 1.0f, 1.0f } }  // Bottom-left vertex (blue)
	};
	UModel model;
	model.Initialize<FVertexSimple>(InDevice, triangle_vertices, 3);
	return model;
}

const UModel UModel::GetDefaultTriangle(ID3D11Device* InDevice)
{
	const static FVertexData triangle_vertices[] = {
	{ {  0.0f,  1.0f, 0.0f }, {  0.5f, 0.5f } }, // Top vertex (red)
	{ {  1.0f, -1.0f, 0.0f }, {  1.0f, 1.0f } }, // Bottom-right vertex (green)
	{ { -1.0f, -1.0f, 0.0f }, {  0.0f, 1.0f } }  // Bottom-left vertex (blue)
	};
	UModel model;
	model.Initialize<FVertexData>(InDevice, triangle_vertices, 3);
	return model;
}

void UModel::Release()
{
	if (VertexBufferInfo.Buffer)
	{
		VertexBufferInfo.Buffer->Release();
	}
}


