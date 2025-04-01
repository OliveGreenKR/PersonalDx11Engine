#pragma once
#include "RenderHardwareInterface.h"

#include "Math.h"
#include <vector>
#include <memory>
#include <map>
#include <stack>
#include <unordered_map>
#include <queue>
#include "RenderContext.h"
#include "TypeCast.h"
#include "RenderDefines.h"
#include "RenderStateInterface.h"

class UModel;
class UShaderBase;
class UGameObject;
class UCamera;
class UPrimitiveComponent;

class URenderer 
{
private:

	//상태별 렌더링 작업 컨테이너
	std::unordered_map<ERenderStateType, 
		std::vector<std::weak_ptr<IRenderData>>> RenderJobs;

	// 단일 렌더링 컨텍스트
	std::unique_ptr<FRenderContext> Context;

	// 상태 객체들
	std::unordered_map<ERenderStateType, std::unique_ptr<IRenderState>> States;

	// 상태 스택
	std::stack<IRenderState*> StateStack;

private:
	// 기본 상태 객체 생성 및 초기화
	void CreateStates();

	//상태별 요청 처리
	void ProcessJobsPerState(const ERenderStateType InState);

	//상태 스택 관리
	void PushState(IRenderState* State);
	void PopState();

public:
	URenderer() = default;
	~URenderer() = default;
	 
public:
	bool Initialize(HWND hWindow, std::shared_ptr<IRenderHardware>& InRenderHardware);
	void Release();

	void BeginFrame();
	void ProcessRender();
	void EndFrame();

	void SubmitJob(const FRenderJob& InJob);

	FRenderContext* GetRenderContext() { return Context.get(); }
};
