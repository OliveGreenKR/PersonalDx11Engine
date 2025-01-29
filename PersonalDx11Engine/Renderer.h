#pragma once
#include "D3D.h"
//ImGui
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_impl_dx11.h"
#include "imGui/imgui_impl_win32.h"

class UModel;
class UShader;
class UGameObject;
class UCamera;

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
	void RenderModel(const UModel* InModel,  UShader* InShader, ID3D11SamplerState* customSampler = nullptr);
	void RenderGameObject(UCamera* InCamera, const UGameObject* InObject,  UShader* InShader, ID3D11SamplerState* customSampler = nullptr);
	void RenderGameObject(UCamera* InCamera, const UGameObject* InObject,  UShader* InShader, ID3D11ShaderResourceView* InTexture, ID3D11SamplerState* customSampler = nullptr);
public:
	__forceinline ID3D11Device* GetDevice() { return RenderHardware->GetDevice(); }
	__forceinline ID3D11DeviceContext* GetDeviceContext() { return RenderHardware->GetDeviceContext(); }
	
	__forceinline ID3D11SamplerState* GetDefaultSamplerState() {return DefaultSamplerState;	}
public:
	__forceinline void SetVSync(bool activation) { RenderHardware->bVSync = activation; }

private:
	bool CreateDefaultSamplerState();

private:
	FD3D* RenderHardware = new FD3D();
	ID3D11SamplerState* DefaultSamplerState = nullptr;

};
