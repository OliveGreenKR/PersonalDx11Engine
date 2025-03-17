#include "Renderer.h"
#include "Model.h"
#include "D3DShader.h"
#include "GameObject.h"
#include "PrimitiveComponent.h"
#include "Camera.h"

void URenderer::Initialize(HWND hWindow, IRenderHardware* InRenderHardware)
{
	assert(InRenderHardware);

	bool result = true;
	RenderHardware = InRenderHardware;
	result = result && RenderHardware->IsDeviceReady();

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

void URenderer::RenderModel(UModel* InModel, UShader* InShader ,ID3D11SamplerState* customSampler)
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

void URenderer::RenderGameObject(UCamera* InCamera, UGameObject* InObject,  UShader* InShader, ID3D11SamplerState* customSampler)
{
	if (!InObject || !InShader || !InObject->IsActive())
		return;


	Matrix ViewMatrix = InCamera->GetViewMatrix();
	Matrix ProjectionMatrix = InCamera->GetProjectionMatrix();

	const auto Primitives = InObject->GetComponentsByType<UPrimitiveComponent>();

	for (auto Primitive : Primitives)
	{
		Matrix WorldMatrix = Primitive->GetTransform()->GetModelingMatrix();
		auto BufferData = FMatrixBufferData(WorldMatrix, ViewMatrix, ProjectionMatrix);
		InShader->BindMatrix(GetDeviceContext(), BufferData);


		FColorBufferData ColorBufferData(Primitive->GetColor());
		InShader->BindColor(GetDeviceContext(), ColorBufferData);
		RenderModel(Primitive->GetModel(), InShader, customSampler);
	}
}
	void URenderer::RenderGameObject(UCamera* InCamera, UGameObject* InObject, UShader* InShader, ID3D11ShaderResourceView* InTexture, ID3D11SamplerState* InCustomSampler)
{
	if (!InObject || !InShader)
		return;

	const auto Primitives = InObject->GetComponentsByType<UPrimitiveComponent>();
	for (auto Primitive : Primitives)
	{
		RenderPrimitve(InCamera, Primitive, InShader, InTexture, InCustomSampler);
	}
}

	void URenderer::RenderPrimitve(UCamera* InCamera, const UPrimitiveComponent* InPrimitive, UShader* InShader, ID3D11ShaderResourceView* InTexture, ID3D11SamplerState* InCustomSampler)
	{
		if ( !InCamera || !InPrimitive || !InShader || !InPrimitive->IsActive())
			return;

		Matrix ViewMatrix = InCamera->GetViewMatrix();
		Matrix ProjectionMatrix = InCamera->GetProjectionMatrix();
		Matrix WorldMatrix = InPrimitive->GetTransform()->GetModelingMatrix();
		auto BufferData = FMatrixBufferData(WorldMatrix, ViewMatrix, ProjectionMatrix);
		InShader->BindMatrix(GetDeviceContext(), BufferData);

		InShader->BindTexture(GetDeviceContext(), InTexture, ETextureSlot::Albedo);
		FColorBufferData ColorBufferData(InPrimitive->GetColor());
		InShader->BindColor(GetDeviceContext(), ColorBufferData);
		RenderModel(InPrimitive->GetModel(), InShader, InCustomSampler);
	}

void URenderer::SubmitRenderJobsInObject(UCamera* InCamera, UGameObject* InObject, ID3D11ShaderResourceView* InTexture)
{
	if (!InCamera || !InObject)
		return;

	const auto Primitives = InObject->GetComponentsByType<UPrimitiveComponent>();

	for (auto Primitive : Primitives)
	{
		SubmitRenderJob(InCamera, Primitive, InTexture);
	}
}

void URenderer::SubmitRenderJob(UCamera* InCamera, UPrimitiveComponent* InPrimitve, ID3D11ShaderResourceView* InTexture)
{
	if (!InCamera || !InPrimitve || !InPrimitve->GetModel())
		return;

	// 렌더 작업 생성 및 추가
	RenderJobs.emplace_back(InCamera, InPrimitve, InTexture);
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

	// 모든 렌더 작업 처리
	for (const auto& Job : RenderJobs)
	{
		RenderPrimitve(Job.Camera, Job.Primitive, InShader, Job.Texture, InCustomSampler);
	}
}

void URenderer::Release()
{
	RenderJobs.clear();
}

