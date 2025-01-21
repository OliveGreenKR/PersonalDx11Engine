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
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;  // 바이리니어 필터링, 부드러운 텍스처 표시
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;     // U좌표 반복
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;     // V좌표 반복  
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;     // W좌표 반복
	samplerDesc.MinLOD = 0;                                // 최소 LOD 레벨
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;               // 최대 LOD 제한 없음
	samplerDesc.MipLODBias = 0;                           // LOD 레벨 조정 없음
	samplerDesc.MaxAnisotropy = 1;                        // 비등방성 필터링 사용 안함
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;  // 비교 샘플링 사용 안함
	
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

