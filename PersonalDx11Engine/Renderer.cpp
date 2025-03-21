#include "Renderer.h"
#include "Model.h"
#include "D3DShader.h"
#include "GameObject.h"
#include "PrimitiveComponent.h"
#include "Camera.h"
#include "RenderStates.h"

bool URenderer::Initialize(HWND hWindow, std::shared_ptr<IRenderHardware>& InRenderHardware)
{
	assert(InRenderHardware);

	bool result = true;

	Context = make_unique<FRenderContext>();

	result = result && InRenderHardware->IsDeviceReady();
	result = result && Context->Initialize(InRenderHardware);

	CreateStates();
	return result;
}

void URenderer::Release()
{
	Context->Release();
}


void URenderer::BeginFrame()
{
	Context->BeginFrame();
}

void URenderer::EndFrame()
{
	Context->EndFrame();
}

void URenderer::SubmitJob(const FRenderJobBase& InJob)
{
	auto state = InJob.GetStateType();
	RenderQueue[state].push_back(InJob);
}

void URenderer::ProcessJobs()
{
	 // 상태별로 렌더링 작업 처리 (상태 전환 최소화)
	for (auto& [stateType, jobs] : RenderQueue)
	{
		if (jobs.empty()) continue;

		// 현재 상태 적용
		Context->PushState(States[stateType].get());

		// 같은 상태의 작업들 일괄 처리
		for (auto& job : jobs)
		{
			job.Execute(Context.get());
		}

		// 상태 복원
		Context->PopState();

		// 처리 완료된 작업 정리
		jobs.clear();
	}
}

void URenderer::CreateStates()
{
	auto Device = Context->GetDevice();
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
		States[ERenderStateType::Solid] = std::move(solidState);

		// 참조 카운트 관리
		solidRasterizerState->Release();
	}


	// 2. 와이어프레임 상태 생성
	D3D11_RASTERIZER_DESC wireframeDesc = {};
	wireframeDesc.FillMode = D3D11_FILL_WIREFRAME;
	wireframeDesc.CullMode = D3D11_CULL_NONE;
	wireframeDesc.DepthClipEnable = TRUE;

	ID3D11RasterizerState* wireframeRasterizerState = nullptr;
	hr = Device->CreateRasterizerState(&wireframeDesc, &wireframeRasterizerState);

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



