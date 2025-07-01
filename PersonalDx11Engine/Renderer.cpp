#include "Renderer.h"
#include "RenderStates.h"
#include "Camera.h"
#include "Debug.h"

URenderer::~URenderer()
{
	RenderJobs.clear();

	for (auto& statesPair : States)
	{
		statesPair.second.reset();
	}
	States.clear();

	RenderDataPool.Reset();

	Context.reset();
}

bool URenderer::Initialize(HWND hWindow, std::shared_ptr<IRenderHardware>& InRenderHardware)
{
	assert(InRenderHardware);
	bool result = true;

	//렌더 컨텍스트
	Context = std::make_unique<FRenderContext>();
	result = result && InRenderHardware->IsDeviceReady();
	result = result && Context->Initialize(InRenderHardware);


	//기본 렌더링 상태 
	CreateStates();

	return result;
}

void URenderer::BeginFrame()
{
	Context->BeginFrame();
}

void URenderer::EndFrame()
{
	Context->EndFrame();
	RenderDataPool.Reset(); //reset memory pool
}

void URenderer::SubmitJob(FRenderJob & InJob)
{
	if (!InJob.RenderData)
		return;

	if (InJob.RenderState == ERenderStateType::Default)
	{
		InJob.RenderState = DefaultState;
	}

	auto& vec = RenderJobs[InJob.RenderState];
	if (vec.capacity() == 0)
	{
		vec.reserve(128);
	}


	vec.push_back(InJob.RenderData);
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
	
	States[ERenderStateType::Solid] = FSolidState::Create(Device);
	States[ERenderStateType::Wireframe] = FWireframeState::Create(Device);
}

void URenderer::ProcessJobsPerState(const ERenderStateType InState)
{
	//상태 적용
	auto* state = States[InState].get();
	PushState(state);

	ID3D11SamplerState* currentSampler = nullptr;
	Context->GetDeviceContext()->PSGetSamplers(0, 1, &currentSampler);
	if (!currentSampler && InState == ERenderStateType::Solid)
	{
		LOG_WARNING("Sampler not Bound in Rendering Sessions");
	}

	for (auto& RenderDataPtr : RenderJobs[InState])
	{
		if (!RenderDataPtr)
			continue;

		//드로우 콜
		Context->DrawRenderData(RenderDataPtr);
	}

	RenderJobs[InState].clear();
	//상태 복원
	PopState();
}



