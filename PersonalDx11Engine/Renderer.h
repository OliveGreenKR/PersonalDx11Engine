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

    FLOAT ClearColor[4] = { 0.025f, 0.025f, 0.025f, 1.0f }; // ȭ���� �ʱ�ȭ(clear)�� �� ����� ���� (RGBA)
    D3D11_VIEWPORT ViewportInfo; // ������ ������ �����ϴ� ����Ʈ ����

};
