#pragma once
#include "RenderHardwareInterface.h"
#include "Math.h"
#include <vector>
#include <memory>
#include <queue>

class UModel;
class UShader;
class UGameObject;
class UCamera;
class UPrimitiveComponent;


// 렌더 작업을 정의하는 구조체
struct FRenderJob
{
	UCamera* Camera;
	const class UPrimitiveComponent* Primitive;
	ID3D11ShaderResourceView* Texture;

	FRenderJob(UCamera* InCamera, const UPrimitiveComponent* InPrimitive,
			   ID3D11ShaderResourceView* InTexture = nullptr)
		: Camera(InCamera)
		, Primitive(InPrimitive)
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

public:
	__forceinline ID3D11Device* GetDevice() { return RenderHardware->GetDevice(); }
	__forceinline ID3D11DeviceContext* GetDeviceContext() { return RenderHardware->GetDeviceContext(); }
	__forceinline ID3D11SamplerState* GetDefaultSamplerState() {return RenderHardware->GetDefaultSamplerState();	}

private:
	IRenderHardware* RenderHardware;
	std::queue<FRenderJob> RenderJobs;
};
