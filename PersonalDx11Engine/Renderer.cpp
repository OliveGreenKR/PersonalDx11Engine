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

	RenderJobs.reserve(512);
}

void URenderer::BindShader(UShader* InShader)
{
	InShader->Bind(RenderHardware->GetDeviceContext());
}

void URenderer::BeforeRender()
{
	RenderHardware->BeginFrame();
}


void URenderer::EndRender()
{
	RenderHardware->EndFrame();
}

void URenderer::RenderModel(UModel* InModel, UShader* InShader, ID3D11SamplerState* customSampler)
{
	if (!InModel || !InModel->IsInitialized()|| !InShader)
		return;

	// 버퍼 매니저에서 해당 모델의 버퍼 리소스 가져오기
	FBufferResource* bufferResource = InModel->GetBufferResource();
	if (!bufferResource || !bufferResource->GetVertexBuffer())
		return;

	// 정점 버퍼 설정
	ID3D11Buffer* vertexBuffer = bufferResource->GetVertexBuffer();
	if (vertexBuffer == nullptr)
	{
		// Handle null buffer error
		return;
	}

	UINT stride = bufferResource->GetStride();
	UINT offset = bufferResource->GetOffset();

	GetDeviceContext()->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);

	// 샘플러 스테이트 설정
	ID3D11SamplerState* samplerState = customSampler ? customSampler : GetDefaultSamplerState();
	GetDeviceContext()->PSSetSamplers(0, 1, &samplerState);

	// 인덱스 버퍼가 있는 경우 인덱스 버퍼 설정 및 DrawIndexed 호출
	if (auto indexBuffer = bufferResource->GetIndexBuffer()) {
		GetDeviceContext()->IASetIndexBuffer(
			indexBuffer,
			DXGI_FORMAT_R32_UINT,
			0
		);
		GetDeviceContext()->DrawIndexed(bufferResource->GetIndexCount(), 0, 0);
	}
	// 인덱스 버퍼가 없는 경우 일반 Draw 호출
	else {
		GetDeviceContext()->Draw(bufferResource->GetVertexCount(), 0);
	}
	
}

void URenderer::RenderGameObject(UCamera* InCamera,const UGameObject* InObject,  UShader* InShader, ID3D11SamplerState* customSampler)
{
	if (!InObject || !InShader || !InObject->IsActive())
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


void URenderer::SubmitRenderJob(UCamera* InCamera, const UGameObject* InObject, ID3D11ShaderResourceView* InTexture)
{
	if (!InCamera || !InObject)
		return;

	// 모델 유효성 검사
	if (!InObject->GetModel() || !InObject->GetModel()->IsInitialized())
		return;

	// 렌더 작업 생성 및 추가
	RenderJobs.emplace_back(InCamera, InObject, InTexture);
}

void URenderer::ClearRenderJobs()
{
	// 렌더 작업 목록 비우기
	RenderJobs.clear();
}
void URenderer::ProcessRenderJobs(UShader* InShader, ID3D11SamplerState* InCustomSampler)
{
	if (!InShader)
		return;

	ID3D11SamplerState* SamplerState = InCustomSampler;
	if (!InCustomSampler)
	{
		SamplerState = GetDefaultSamplerState();
	}

	// 셰이더 바인딩
	BindShader(InShader);

	// 모든 렌더 작업 처리
	for (const auto& Job : RenderJobs)
	{
		RenderGameObject(Job.Camera, Job.GameObject, InShader, SamplerState);
	}
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

