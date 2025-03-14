#pragma once
#include "RenderHardwareInterface.h"
#include "Math.h"
#include <vector>

class UModel;
class UShader;
class UGameObject;
class UCamera;

// 렌더 작업을 정의하는 구조체
struct FRenderJob
{
	UCamera* Camera;
	const UGameObject* GameObject;
	ID3D11ShaderResourceView* Texture;

	FRenderJob(UCamera* InCamera, const UGameObject* InGameObject,
			   ID3D11ShaderResourceView* InTexture = nullptr)
		: Camera(InCamera)
		, GameObject(InGameObject)
		, Texture(InTexture)
	{
	}
};

struct FVertexSimple
{
	float x, y, z;    // Position
	float r, g, b, a; // Color
};

class URenderer 
{

public:
	URenderer() = default;
	~URenderer() = default;

public:
	void Initialize(HWND hWindow);
	void Release();

	void BindShader(UShader* InShader);
	void BeforeRender();
	void EndRender();

	void RenderModel(UModel* InModel,  UShader* InShader, ID3D11SamplerState* customSampler = nullptr);

	void RenderGameObject(UCamera* InCamera, const UGameObject* InObject,  UShader* InShader, ID3D11SamplerState* customSampler = nullptr);
	void RenderGameObject(UCamera* InCamera, const UGameObject* InObject,  UShader* InShader, ID3D11ShaderResourceView* InTexture, ID3D11SamplerState* InCustomSampler = nullptr);
	
	void SubmitRenderJob(UCamera* InCamera, const UGameObject* InObject, ID3D11ShaderResourceView* InTexture);
	void ClearRenderJobs();
	void ProcessRenderJobs(UShader* InShader, ID3D11SamplerState* InCustomSampler = nullptr);

public:
	__forceinline ID3D11Device* GetDevice() { return RenderHardware->GetDevice(); }
	__forceinline ID3D11DeviceContext* GetDeviceContext() { return RenderHardware->GetDeviceContext(); }
	__forceinline ID3D11SamplerState* GetDefaultSamplerState() {return RenderHardware->GetDefaultSamplerState();	}

private:
	IRenderHardware* RenderHardware;
	std::vector<FRenderJob> RenderJobs;
};
