#pragma once
#include "D3D.h";
#include "D3DShader.h"
//ImGui
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_impl_dx11.h"
#include "imGui/imgui_impl_win32.h"

#include "Vector.h"


struct FVertexSimple
{
    float x, y, z;    // Position
    float r, g, b, a; // Color
};

class URenderer 
{
    struct FConstants
    {
        FVector Offset;
        float pad;
    };

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
    //ID3D11Buffer* CreateVertexBuffer(FVertexSimple* vertices, UINT byteWidth);
    //void ReleaseVertexBuffer(ID3D11Buffer* vertexBuffer) { vertexBuffer->Release(); }

    void RenderPrimitive(ID3D11Buffer* pBuffer, UINT numVertices);
    void UpdateConstant(FVector InOffset);

private:
    void CreateConstantBuffer();


private:
    FD3D* RenderHardware = new FD3D();
    FD3DShader* Shader = new FD3DShader();


#pragma region shader
private:
    //todo: shader �������̽��� ��� �и�?
    //ID3D11VertexShader* SimpleVertexShader;
    //ID3D11PixelShader* SimplePixelShader;
    //ID3D11InputLayout* SimpleInputLayout;
    //unsigned int Stride;

    //void CreateShader();
    //void ReleaseShader();
    //ID3D11Buffer* ConstantBuffer = nullptr; // ���̴��� �����͸� �����ϱ� ���� ��� ����
#pragma endregion

    FLOAT ClearColor[4] = { 0.025f, 0.025f, 0.025f, 1.0f }; // ȭ���� �ʱ�ȭ(clear)�� �� ����� ���� (RGBA)
    D3D11_VIEWPORT ViewportInfo; // ������ ������ �����ϴ� ����Ʈ ����

};
