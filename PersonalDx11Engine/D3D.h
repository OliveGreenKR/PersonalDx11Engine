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
    void Shutdown();

    void BeginScene();
    void EndScene();


    __forceinline ID3D11Device* GetDevice()                 { return Device; }
    __forceinline ID3D11DeviceContext* GetDeviceContext()   { return DeviceContext; }

    void GetProjectionMatrix(OUT XMMATRIX& OutMatrix)       { OutMatrix = MatirxProjection;  }
    void GetWorldMatrix(OUT XMMATRIX& OutMatrix)            { OutMatrix = MatrixWorld; }
    void GetOrthoMatrix(OUT XMMATRIX& OutMatrix)            { OutMatrix = MatrixOrtho; }

public:
    bool bVSync = true;

private:

    void PrepareRender();
   
    bool CreateDeviceAndSwapChain(HWND Hwnd);
    bool CreateFrameBuffer();
    bool CreateRasterizerStateAndMatricies();

    void ReleaseDeviceAndSwapChain();
    void ReleaseFrameBuffer();
    void ReleaseRasterizerState();
private:

    ID3D11Device* Device = nullptr;
    ID3D11DeviceContext* DeviceContext = nullptr;
    IDXGISwapChain* SwapChain = nullptr;
    //RTV는 렌더링 결과물을 저장할 메모리 영역 지정, 
    ID3D11RenderTargetView* renderTargetView =  nullptr;
    ID3D11DepthStencilView* depthStencilView = nullptr;

    ID3D11Texture2D* FrameBuffer = nullptr;
    ID3D11RenderTargetView* FrameBufferRTV = nullptr;
    ID3D11RasterizerState* RasterizerState = nullptr;


    XMMATRIX MatirxProjection;
    XMMATRIX MatrixWorld;
    XMMATRIX MatrixOrtho;

    D3D11_VIEWPORT ViewportInfo;
    FLOAT ClearColor[4] = { 0.025f, 0.025f, 0.025f, 1.0f };
};