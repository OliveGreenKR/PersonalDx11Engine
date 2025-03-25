#pragma once
#include "RenderHardwareInterface.h"

#include "Math.h"
#include <vector>
#include <memory>
#include <map>
#include <unordered_map>
#include <queue>
#include "RenderContext.h"
#include "RenderJobs.h"
#include "FrameMemoryPool.h"
#include "TypeCast.h"

class UModel;
class UShader;
class UGameObject;
class UCamera;
class UPrimitiveComponent;


class URenderer 
{
private:
	//상태별 렌더링 큐
	std::map<ERenderStateType, std::vector<FRenderJobBase*>> RenderQueue;
	// 단일 렌더링 컨텍스트
	std::unique_ptr<FRenderContext> Context;
	// 상태 객체들
	std::unordered_map<ERenderStateType, std::unique_ptr<IRenderState>> States;

	std::unique_ptr<FFrameMemoryPool> JobPool;

private:
	//기본 상태 객체 생성 및 초기화
	void CreateStates();

public:
	URenderer() = default;
	~URenderer() = default;

public:
	bool Initialize(HWND hWindow, std::shared_ptr<IRenderHardware>& InRenderHardware);
	void Release();

	void BeginFrame();
	void ProcessJobs();
	void EndFrame();

	template<typename T>
	T* AcquireJob()
	{
		static_assert(std::is_base_of_v<FRenderJobBase, T>);
		return JobPool->Allocate<T>();
	}
	/// <summary>
	/// Do not use Job with direct allocation, Use 'AcquireJob' Instead
	/// </summary>
	void SubmitJob(FRenderJobBase* InJob);

	ID3D11SamplerState* GetDefaultSamplerState() { return Context->GetDefaultSamplerState(); }
	FRenderContext* GetRenderContext() { return Context.get(); }
};
