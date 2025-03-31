#include "Renderer.h"
#include "RenderStates.h"

bool URenderer::Initialize(HWND hWindow, std::shared_ptr<IRenderHardware>& InRenderHardware)
{
	assert(InRenderHardware);

	bool result = true;

	Context = std::make_unique<FRenderContext>();

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

void URenderer::SubmitJob(const FRenderJob& InJob)
{
	if (InJob.RenderState == ERenderStateType::None ||
		InJob.RenderData.expired())
		return;

	RenderJobs[InJob.RenderState].push_back(InJob.RenderData);
}

void URenderer::ProcessRender()
{
	if (!Context)
		return;

	ProcessJobsPerState(ERenderStateType::Solid);
	ProcessJobsPerState(ERenderStateType::Wireframe);
}

void URenderer::PushState(IRenderState* State)
{
	ID3D11DeviceContext* DeviceContext = GetRenderContext()->GetDeviceContext();
	if (!State || !DeviceContext) return;

	// 상태 스택에 푸시
	State->Apply(DeviceContext);
	StateStack.push(State);
}

void URenderer::PopState()
{
	ID3D11DeviceContext* DeviceContext = GetRenderContext()->GetDeviceContext();
	if (StateStack.empty() || !DeviceContext) return;

	IRenderState* CurrentState = StateStack.top();
	StateStack.pop();

	// 현재 상태 복원
	if (CurrentState)
	{
		CurrentState->Restore(DeviceContext);
	}

	// 이전 상태가 있다면 다시 적용
	if (!StateStack.empty())
	{
		StateStack.top()->Apply(DeviceContext);
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
		solidRasterizerState = nullptr;
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
		wireframeRasterizerState = nullptr;
	}
}

void URenderer::ProcessJobsPerState(const ERenderStateType InState)
{
	//상태 적용
	auto* state = States[InState].get();
	PushState(state);


	for (auto& RenderDataPtr : RenderJobs[InState])
	{
		if (RenderDataPtr.expired())
			continue;
		//드로우 콜
		Context->DrawRenderData(RenderDataPtr.lock().get());
	}

	RenderJobs[InState].clear();
	//상태 복원
	PopState();
}



