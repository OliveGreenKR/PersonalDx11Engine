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
	IRenderHardware* RenderHardware;
	std::queue<FRenderJob> RenderJobs;

	//상태별 렌더링 큐
	std::map<ERenderStateType, std::vector<FRenderJob>> RenderQueue;
	
	// 단일 렌더링 컨텍스트
	FRenderContext Context;

	// 상태 객체들
	std::unordered_map<ERenderStateType, std::unique_ptr<IRenderState>> States;

public:
	URenderer() = default;
	~URenderer() = default;

public:
	void Initialize(HWND hWindow, IRenderHardware* InRenderHardware);
	void Release();

	void BindShader(UShader* InShader);
	void BeforeRender();
	void EndRender();

	void RenderModel(UModel* InModel,  UShader* InShader, ID3D11SamplerState * customSampler = nullptr);

	void RenderGameObject(UCamera* InCamera, UGameObject* InObject,  UShader* InShader, ID3D11SamplerState* customSampler = nullptr);
	void RenderGameObject(UCamera* InCamera, UGameObject* InObject,  UShader* InShader, ID3D11ShaderResourceView* InTexture, ID3D11SamplerState* InCustomSampler = nullptr);
	void RenderPrimitve(UCamera* InCamera, const UPrimitiveComponent* InPrimitive, UShader* InShader, ID3D11ShaderResourceView* InTexture, ID3D11SamplerState* InCustomSampler = nullptr);
	void SubmitRenderJobsInObject(UCamera* InCamera, UGameObject* InObject, ID3D11ShaderResourceView* InTexture);
	void SubmitRenderJob(UCamera* InCamera, UPrimitiveComponent* InPrimitve, ID3D11ShaderResourceView* InTexture);
	void ClearRenderJobs();
	void ProcessRenderJobs(UShader* InShader, ID3D11SamplerState* InCustomSampler = nullptr);

	//컨텍스트, 스테이트 추상화
	void SubmitRenderJob(const FRenderJob& InJob);
	void ProcessRenderJobs();

	void BindShader(ID3D11VertexShader* VS, ID3D11PixelShader* PS);

private:
	void CreateStates();

public:
	__forceinline ID3D11Device* GetDevice() { return RenderHardware->GetDevice(); }
	__forceinline ID3D11DeviceContext* GetDeviceContext() { return RenderHardware->GetDeviceContext(); }
	__forceinline ID3D11SamplerState* GetDefaultSamplerState() {return RenderHardware->GetDefaultSamplerState();	}

};
