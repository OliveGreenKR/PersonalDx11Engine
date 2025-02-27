#include "Renderer.h"
#include "Model.h"
#include "D3DShader.h"
#include "GameObject.h"
#include "Camera.h"

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

void URenderer::RenderModel(UModel* InModel, UShader* InShader, ID3D11SamplerState* customSampler)
{
	if (!InModel || !InModel->IsInitialized()|| !InShader)
		return;

	//정점 정보
	const FVertexDataContainer* vertexData = UModelBufferManager::Get()->GetVertexDataByHash(InModel->GetDataHash());
	
	//모델 정점  입력 레이아웃 쉐이더 설정
	ID3D11InputLayout** outLayout;
	const void* shaderBytecode;
	size_t bytecodeLength;
	InShader->GetShaderBytecode(&shaderBytecode, &bytecodeLength);
	vertexData->GetOrCreateInputLayout(GetDevice(), shaderBytecode, bytecodeLength);

	// 버퍼 매니저에서 해당 모델의 버퍼 리소스 가져오기
	const FBufferResource* bufferResource = UModelBufferManager::Get()->GetBufferByHash(InModel->GetDataHash());
	if (!bufferResource || !bufferResource->GetVertexBuffer())
		return;

	// 정점 버퍼 설정
	ID3D11Buffer* vertexBuffer = bufferResource->GetVertexBuffer();
	if (vertexBuffer == nullptr)
	{
		// Handle null buffer error
		return;
	}

	UINT stride = vertexData->GetStride();
	UINT offset = 0;

	GetDeviceContext()->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);

	// 샘플러 스테이트 설정
	ID3D11SamplerState* samplerState = customSampler ? customSampler : GetDefaultSamplerState();
	GetDeviceContext()->PSSetSamplers(0, 1, &samplerState);

	// 인덱스 버퍼가 있는 경우 인덱스 버퍼 설정 및 DrawIndexed 호출
	if (bufferResource->HasIndexBuffer()) {
		GetDeviceContext()->IASetIndexBuffer(
			bufferResource->GetIndexBuffer(),
			DXGI_FORMAT_R32_UINT,
			0
		);
		GetDeviceContext()->DrawIndexed(vertexData->GetIndexCount(), 0, 0);
	}
	// 인덱스 버퍼가 없는 경우 일반 Draw 호출
	else {
		GetDeviceContext()->Draw(vertexData->GetVertexCount(), 0);
	}
}

void URenderer::RenderGameObject(UCamera* InCamera,const UGameObject* InObject,  UShader* InShader, ID3D11SamplerState* customSampler)
{
	if (!InObject || !InShader)
		return;

	Matrix WorldMatrix = InObject->GetWorldMatrix();
	Matrix ViewMatrix = InCamera->GetViewMatrix();
	Matrix ProjectionMatrix = InCamera->GetProjectionMatrix();
	auto BufferData = FMatrixBufferData(WorldMatrix, ViewMatrix, ProjectionMatrix);

	InShader->BindMatrix(GetDeviceContext(), BufferData);

	RenderModel(InObject->GetModel(), InShader, customSampler);
}

void URenderer::RenderGameObject(UCamera* InCamera, const UGameObject* InObject, UShader* InShader, ID3D11ShaderResourceView* InTexture, ID3D11SamplerState* InCustomSampler)
{
	if (!InObject || !InShader)
		return;

	Vector4 Color;
	Color = InObject->bDebug ? InObject->GetDebugColor() : Color::White();
	FDebugBufferData DebugBufferData(Color);
	InShader->BindColor(GetDeviceContext(), DebugBufferData);
	InShader->BindTexture(GetDeviceContext(), InTexture, ETextureSlot::Albedo);
	RenderGameObject(InCamera, InObject, InShader, InCustomSampler);
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

