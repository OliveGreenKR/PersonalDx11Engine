#pragma once
#include "D3D.h"
#include "D3DShader.h"
//ImGui
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_impl_dx11.h"
#include "imGui/imgui_impl_win32.h"

class UModel;

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

	void BeforeRender();

	void EndRender();

	void Release();

public:
	__forceinline ID3D11Device* GetDevice() { return RenderHardware->GetDevice(); }
	__forceinline ID3D11DeviceContext* GetDeviceContext() { return RenderHardware->GetDeviceContext(); }

public:
	__forceinline void SetVSync(bool activation) { RenderHardware->bVSync = activation; }


public:
	void RenderModel(const UModel& InModel);


private:
	FD3D* RenderHardware = new FD3D();
	FD3DShader* Shader = new FD3DShader();

};
