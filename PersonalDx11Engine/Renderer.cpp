#include "Renderer.h"
#include "Model.h"

void URenderer::Initialize(HWND hWindow)
{
	bool result;
	result = RenderHardware->Initialize(hWindow);

	assert(result);

	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	result = Shader->Initialize(RenderHardware->GetDevice(), L"ShaderW0.hlsl", L"ShaderW0.hlsl", layout, ARRAYSIZE(layout));
}

void URenderer::BeforeRender()
{
	RenderHardware->BeginScene();
	Shader->Bind(RenderHardware->GetDeviceContext());
}


void URenderer::EndRender()
{
	RenderHardware->EndScene();
}

void URenderer::Release()
{
	Shader->Release();
	RenderHardware->Release();
}

void URenderer::RenderModel(const UModel& InModel)
{
	UINT offset = 0;

	assert(InModel.IsIntialized());

	VertexBufferInfo vertexBuffer = InModel.GetVertexBufferInfo();
	GetDeviceContext()->IASetVertexBuffers(0, 1, &vertexBuffer.Buffer, &vertexBuffer.Stride, &offset);
	GetDeviceContext()->Draw(vertexBuffer.NumVertices, 0);
	//ID3D11Buffer* pBuffer = InModel.GetVertexBuffer();
	//UINT stride = InModel.GetVertexStride();
	//GetDeviceContext()->IASetVertexBuffers(0, 1, &pBuffer, &stride, &offset);

	//UINT numVertices = InModel.GetNumVertices();
	//GetDeviceContext()->Draw(numVertices, 0);
}
