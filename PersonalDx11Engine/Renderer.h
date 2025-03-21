#pragma once
#include "RenderHardwareInterface.h"

#include "Math.h"
#include <vector>
#include <memory>
#include <map>
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
	std::queue<IRenderJob> RenderJobs;

	//상태별 렌더링 큐
	std::map<ERenderStateType, std::vector<IRenderJob>> RenderQueue;
	
	// 단일 렌더링 컨텍스트
	FRenderContext Context;

	// 렌더링 하드웨어
	std::shared_ptr<IRenderHardware> RenderHardware;

	// 상태 객체들
	std::unordered_map<ERenderStateType, std::unique_ptr<IRenderState>> States;
public:
	URenderer() = default;
	~URenderer() = default;

public:
	void Initialize(HWND hWindow, std::shared_ptr<IRenderHardware>& InRenderHardware);
	void Release();

	void BeginFrame();
	void ProcessJobs();
	void EndFrame();

	// 렌더 작업 제출
	void SubmitJob(const IRenderJob& InJob);

	//컨텍스트, 스테이트 추상화
	void SubmitRenderJob(const IRenderJob& InJob);


	void BindShader(ID3D11VertexShader* VS, ID3D11PixelShader* PS);

private:
	void CreateStates();

public:
	__forceinline ID3D11Device* GetDevice() { return RenderHardware->GetDevice(); }
	__forceinline ID3D11DeviceContext* GetDeviceContext() { return Context.GetDeviceContext(); }
	__forceinline ID3D11SamplerState* GetDefaultSamplerState() {return RenderHardware->GetDefaultSamplerState();	}

};
