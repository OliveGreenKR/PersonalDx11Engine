#include "myd3d.h"
#include "Vector.h"

class FD3DRenderer 
{
public:
    FD3DRenderer() = default;
    ~FD3DRenderer() = default;

public:
    void Intialize(HWND hWindow);
    void Tick();
    void Shutdown();

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

    // ���� ü���� �� ���ۿ� ����Ʈ ���۸� ��ü�Ͽ� ȭ�鿡 ���
    void SwapBuffer();
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

private:
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

private:
    UINT bVSync = 1; // 1: VSync Ȱ��ȭ
};
