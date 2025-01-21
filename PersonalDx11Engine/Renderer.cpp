#include "Renderer.h"
#include "Model.h"
#include "D3DShader.h"
#include "GameObject.h"

void URenderer::Initialize(HWND hWindow)
{
	bool result;
	result = RenderHardware->Initialize(hWindow);
	assert(result);
	result = CreateDefaultSamplerState();
	assert(result);
	
}

void URenderer::BindShader(UShader* InShader)
{
	InShader->Bind(RenderHardware->GetDeviceContext());
}

void URenderer::BeforeRender()
{
	RenderHardware->BeginScene();
}


void URenderer::EndRender()
{
	RenderHardware->EndScene();
}

void URenderer::RenderModel(const UModel* InModel, const UShader* InShader, ID3D11SamplerState* customSampler)
{
	UINT offset = 0;

	assert(InModel->IsIntialized());

	VertexBufferInfo vertexBuffer = InModel->GetVertexBufferInfo();
	GetDeviceContext()->IASetVertexBuffers(0, 1, &vertexBuffer.Buffer, &vertexBuffer.Stride, &offset);
	GetDeviceContext()->Draw(vertexBuffer.NumVertices, 0);
}

void URenderer::RenderGameObject(const UGameObject* InObject, const UShader* InShader, ID3D11SamplerState* customSampler)
{
	UINT offset = 0;
	
	assert(InObject);

	RenderModel(InObject->GetModel(), InShader, customSampler);
}

bool URenderer::CreateDefaultSamplerState()
{
	D3D11_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;  // ���̸��Ͼ� ���͸�, �ε巯�� �ؽ�ó ǥ��
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;     // U��ǥ �ݺ�
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;     // V��ǥ �ݺ�  
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;     // W��ǥ �ݺ�
	samplerDesc.MinLOD = 0;                                // �ּ� LOD ����
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;               // �ִ� LOD ���� ����
	samplerDesc.MipLODBias = 0;                           // LOD ���� ���� ����
	samplerDesc.MaxAnisotropy = 1;                        // ���漺 ���͸� ��� ����
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;  // �� ���ø� ��� ����
	
	HRESULT result = GetDevice()->CreateSamplerState(&samplerDesc, &DefaultSamplerState);
	return SUCCEEDED(result);
}

void URenderer::Release()
{
	if (DefaultSamplerState)
	{
		DefaultSamplerState->Release();
		DefaultSamplerState = nullptr;
	}
	RenderHardware->Release();
}

