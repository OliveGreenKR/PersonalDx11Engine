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

class UModel;
class UShader;
class UGameObject;
class UCamera;
class UPrimitiveComponent;

struct FVertexSimple
{
	float x, y, z;    // Position
	float r, g, b, a; // Color
};

class URenderer 
{
private:
	std::queue<FRenderJobBase> RenderJobs;

	//상태별 렌더링 큐
	std::map<ERenderStateType, std::vector<FRenderJobBase>> RenderQueue;
	
	// 단일 렌더링 컨텍스트
	std::unique_ptr<FRenderContext> Context;

	// 상태 객체들
	std::unordered_map<ERenderStateType, std::unique_ptr<IRenderState>> States;
public:
	URenderer() = default;
	~URenderer() = default;

public:
	bool Initialize(HWND hWindow, std::shared_ptr<IRenderHardware>& InRenderHardware);
	void Release();

	void BeginFrame();
	void ProcessJobs();
	void EndFrame();

	// 렌더 작업 제출
	void SubmitJob(const FRenderJobBase& InJob);

private:
	void CreateStates();

public:
	__forceinline FRenderContext* GetRenderContext() { return Context.get(); }

};
