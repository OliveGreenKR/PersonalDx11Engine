#pragma once
//link for D3D
#include "D3D.h";
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
    void Shutdown();

    ID3D11Buffer* CreateVertexBuffer(FVertexSimple* vertices, UINT byteWidth);
    void ReleaseVertexBuffer(ID3D11Buffer* vertexBuffer) { vertexBuffer->Release(); }


    void PrepareRender();
    void PrepareShader();
    void RenderPrimitive(ID3D11Buffer* pBuffer, UINT numVertices);
    void SwapBuffer();
    void UpdateConstant(FVector InOffset);

private:
    void CreateConstantBuffer();


private:
#pragma region d3d
    // ������ �ʱ�ȭ �Լ�
    void Create(HWND hWindow);

    // Direct3D ��ġ �� ���� ü���� �����ϴ� �Լ�
    void CreateDeviceAndSwapChain(HWND hWindow);

    // Direct3D ��ġ �� ���� ü���� �����ϴ� �Լ�
    void ReleaseDeviceAndSwapChain();

    // ������ ���۸� �����ϴ� �Լ�
    void CreateFrameBuffer();

    // ������ ���۸� �����ϴ� �Լ�
    void ReleaseFrameBuffer();

    // �����Ͷ����� ���¸� �����ϴ� �Լ�
    void CreateRasterizerState();

    // �����Ͷ����� ���¸� �����ϴ� �Լ�
    void ReleaseRasterizerState();

    // �������� ���� ��� ���ҽ��� �����ϴ� �Լ�
    void Release();
#pragma endregion

#pragma region shader
private:
    //todo: shader �������̽��� ��� �и�?
    ID3D11VertexShader* SimpleVertexShader;
    ID3D11PixelShader* SimplePixelShader;
    ID3D11InputLayout* SimpleInputLayout;
    unsigned int Stride;

    void CreateShader();
    void ReleaseShader();
#pragma endregion

public:
    // Direct3D 11 ��ġ(Device)�� ��ġ ���ؽ�Ʈ(Device Context) �� ���� ü��(Swap Chain)�� �����ϱ� ���� �����͵�
    ID3D11Device* Device = nullptr; // GPU�� ����ϱ� ���� Direct3D ��ġ
    ID3D11DeviceContext* DeviceContext = nullptr; // GPU ��� ������ ����ϴ� ���ؽ�Ʈ
    IDXGISwapChain* SwapChain = nullptr; // ������ ���۸� ��ü�ϴ� �� ���Ǵ� ���� ü��

    // �������� �ʿ��� ���ҽ� �� ���¸� �����ϱ� ���� ������
    ID3D11Texture2D* FrameBuffer = nullptr; // ȭ�� ��¿� �ؽ�ó
    ID3D11RenderTargetView* FrameBufferRTV = nullptr; // �ؽ�ó�� ���� Ÿ������ ����ϴ� ��
    ID3D11RasterizerState* RasterizerState = nullptr; // �����Ͷ����� ����(�ø�, ä��� ��� �� ����)
    ID3D11Buffer* ConstantBuffer = nullptr; // ���̴��� �����͸� �����ϱ� ���� ��� ����

    FLOAT ClearColor[4] = { 0.025f, 0.025f, 0.025f, 1.0f }; // ȭ���� �ʱ�ȭ(clear)�� �� ����� ���� (RGBA)
    D3D11_VIEWPORT ViewportInfo; // ������ ������ �����ϴ� ����Ʈ ����

public:
    UINT bVSync = 1; // 1: VSync Ȱ��ȭ
};
