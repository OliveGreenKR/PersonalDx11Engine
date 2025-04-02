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
#include "FrameMemoryPool.h"

class UModel;
class UShaderBase;
class UGameObject;
class UCamera;
class UPrimitiveComponent;

class URenderer 
{
private:

	//상태별 렌더링 작업 큐
	std::unordered_map<ERenderStateType, 
		std::vector<IRenderData*>> RenderJobs;

	// 단일 렌더링 컨텍스트
	std::unique_ptr<FRenderContext> Context;

	// 상태 객체들
	std::unordered_map<ERenderStateType, std::unique_ptr<IRenderState>> States;

	// 상태 스택
	std::stack<IRenderState*> StateStack;

	//렌더 데이터 풀
	FFrameMemoryPool RenderDataPool = FFrameMemoryPool(8 * 1024 * 1024);
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

	template< typename T , typename =  
		std::enable_if_t<std::is_base_of_v<IRenderData,T> ||
				std::is_same_v<IRenderData,T>>>
	T* AllocateRenderData()
	{
		return RenderDataPool.Allocate<T>();
	}

	FRenderContext* GetRenderContext() { return Context.get(); }
};
