#include "Renderer.h"
#include "Model.h"
#include "D3DShader.h"
#include "GameObject.h"
#include "PrimitiveComponent.h"
#include "Camera.h"
#include "RenderStates.h"

void URenderer::Initialize(HWND hWindow, IRenderHardware* InRenderHardware)
{
	assert(InRenderHardware);

	bool result = true;
	RenderHardware = InRenderHardware;
	Context.SetDeviceContext(InRenderHardware->GetDeviceContext());
	result = result && RenderHardware->IsDeviceReady();
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
		Matrix WorldMatrix = Primitive->GetWorldTransform().GetModelingMatrix();
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
		Matrix WorldMatrix = InPrimitive->GetWorldTransform().GetModelingMatrix();
		auto BufferData = FMatrixBufferData(WorldMatrix, ViewMatrix, ProjectionMatrix);
		InShader->BindMatrix(GetDeviceContext(), BufferData);

		InShader->BindTexture(GetDeviceContext(), InTexture, ETextureSlot::Albedo);
		FColorBufferData ColorBufferData(InPrimitive->GetColor());
		InShader->BindColor(GetDeviceContext(), ColorBufferData);
		RenderModel(InPrimitive->GetModel(), InShader, InCustomSampler);
	}

void URenderer::SubmitRenderJob(const FRenderJob& InJob)
{
	RenderQueue[InJob.StateType].push_back(InJob);
}

void URenderer::ProcessRenderJobs()
{
	 // 상태별로 렌더링 작업 처리 (상태 전환 최소화)
	for (auto& [stateType, jobs] : RenderQueue)
	{
		if (jobs.empty()) continue;

		// 현재 상태 적용
		Context.PushState(States[stateType].get());

		// 같은 상태의 작업들 일괄 처리
		for (auto& job : jobs)
		{
			job.Execute(Context);
		}

		// 상태 복원
		Context.PopState();

		// 처리 완료된 작업 정리
		jobs.clear();
	}
}

void URenderer::BindShader(ID3D11VertexShader* VS, ID3D11PixelShader* PS)
{
	Context.BindShader(VS, PS);
}

void URenderer::CreateStates()
{
	auto Device = RenderHardware->GetDevice();
	if (!Device) return;

	// 1. 기본 상태 생성
	D3D11_RASTERIZER_DESC solidDesc = {};
	solidDesc.FillMode = D3D11_FILL_SOLID; 
	solidDesc.CullMode = D3D11_CULL_BACK; 

	ID3D11RasterizerState* solidRasterizerState = nullptr;
	HRESULT hr = Device->CreateRasterizerState(&solidDesc, &solidRasterizerState);

	if (SUCCEEDED(hr))
	{
		// 상태 객체 생성 및 저장
		auto solidState = std::make_unique<FSolidState>();
		solidState->SetSolidRSS(solidRasterizerState);
		States[ERenderStateType::Default] = std::move(solidState);

		// 참조 카운트 관리
		solidRasterizerState->Release();
	}


	// 2. 와이어프레임 상태 생성
	D3D11_RASTERIZER_DESC wireframeDesc = {};
	wireframeDesc.FillMode = D3D11_FILL_WIREFRAME;
	wireframeDesc.CullMode = D3D11_CULL_NONE;
	wireframeDesc.DepthClipEnable = TRUE;

	ID3D11RasterizerState* wireframeRasterizerState = nullptr;
	HRESULT hr = Device->CreateRasterizerState(&wireframeDesc, &wireframeRasterizerState);

	if (SUCCEEDED(hr))
	{
		// 상태 객체 생성 및 저장
		auto wireframeState = std::make_unique<FWireframeState>();
		wireframeState->SetWireFrameRSState(wireframeRasterizerState);
		States[ERenderStateType::Wireframe] = std::move(wireframeState);

		// 참조 카운트 관리를 위해 Release
		wireframeRasterizerState->Release();
	}
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
	RenderJobs.push({ InCamera, InPrimitve, InTexture });
}

void URenderer::ClearRenderJobs()
{
	while (!RenderJobs.empty())
	{
		// 렌더 작업 목록 비우기
		RenderJobs.pop();
	}
}

void URenderer::ProcessRenderJobs(UShader* InShader, ID3D11SamplerState* InCustomSampler)
{
	if (!InShader)
		return;

	// 모든 렌더 작업 처리
	while (!RenderJobs.empty())
	{
		const auto& Job = RenderJobs.front();
		RenderJobs.pop();
		RenderPrimitve(Job.Camera, Job.Primitive, InShader, Job.Texture, InCustomSampler);
	}
}

void URenderer::Release()
{

}

