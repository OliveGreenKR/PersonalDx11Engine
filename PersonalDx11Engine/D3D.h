#pragma once
#pragma comment(lib, "user32")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")
#pragma comment(lib, "dxguid")

#include <d3d11.h>
#include <d3dcompiler.h>
#include <directxmath.h>

using namespace DirectX;

class FD3D
{
public:
    FD3D() = default;
    ~FD3D();

    bool Initialize(HWND Hwnd);
    void Release();

    void BeginScene();
    void EndScene();


    __forceinline ID3D11Device* GetDevice()                 { return Device; }
    __forceinline ID3D11DeviceContext* GetDeviceContext()   { return DeviceContext; }

    bool CopyBuffer(ID3D11Buffer* SrcBuffer, OUT ID3D11Buffer** DestBuffer);

public:
    bool bVSync = true;

private:

    void PrepareRender();
   
    bool CreateDeviceAndSwapChain(HWND Hwnd);
    bool CreateFrameBuffer();
    bool CreateRasterizerState();
    bool CreateDpethStencilBuffer();
    bool CreateDepthStencilState();
    bool CreateDepthStencillView();

    void ReleaseDeviceAndSwapChain();
    void ReleaseFrameBuffer();
    void ReleaseRasterizerState();
    void ReleaseDepthStencil();
private:

    ID3D11Device* Device = nullptr;
    ID3D11DeviceContext* DeviceContext = nullptr;
    IDXGISwapChain* SwapChain = nullptr;

    ID3D11DepthStencilView* DepthStencilView = nullptr;
    ID3D11Texture2D* DepthStencilBuffer = nullptr;
    ID3D11DepthStencilState* DepthStencilState = nullptr;
    ID3D11Texture2D* FrameBuffer = nullptr;
    //RenderTargetView는 렌더링 결과물을 저장할 메모리 영역 지정, 
    ID3D11RenderTargetView* FrameBufferRTV = nullptr;
    ID3D11RasterizerState* RasterizerState = nullptr;

    D3D11_VIEWPORT ViewportInfo;
    FLOAT ClearColor[4] = { 0.025f, 0.025f, 0.025f, 1.0f };
};