#pragma once
#include "D3D.h"
#include "D3DShader.h"
//ImGui
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_impl_dx11.h"
#include "imGui/imgui_impl_win32.h"

#include "Vector.h"

class URenderer 
{

public:
    URenderer() = default;
    ~URenderer() = default;

public:
    void Initialize(HWND hWindow);

    void BeginRender();
    void Render();
    void EndRender();

    void Shutdown();

public:
    __forceinline void SetVSync(bool activation) { RenderHardware->bVSync = activation; }


public:
    //void RenderPrimitive(ID3D11Buffer* pBuffer, UINT numVertices);
    //void UpdateConstant(FVector InOffset);


private:
    FD3D* RenderHardware = new FD3D();
    FD3DShader* Shader = new FD3DShader();

    FLOAT ClearColor[4] = { 0.025f, 0.025f, 0.025f, 1.0f }; // 화면을 초기화(clear)할 때 사용할 색상 (RGBA)
    D3D11_VIEWPORT ViewportInfo; // 렌더링 영역을 정의하는 뷰포트 정보

};
