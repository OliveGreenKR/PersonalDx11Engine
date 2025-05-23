#pragma once
#include "RenderHardwareInterface.h"

using namespace DirectX;

class FD3D : public IRenderHardware
{
public:
    FD3D() = default;
    ~FD3D();

    // Inherited via IRenderHardware
    bool Initialize(HWND Hwnd) override;
    void Release() override;
    void BeginFrame() override;
    void EndFrame() override;
     
    bool IsDeviceReady() override { return bIsInitialized; }

    __forceinline ID3D11Device* GetDevice() override               { return Device; }
    __forceinline ID3D11DeviceContext* GetDeviceContext() override   { return DeviceContext; }

    bool CopyBuffer(ID3D11Buffer* SrcBuffer, OUT ID3D11Buffer** DestBuffer);

    void SetVSync(const bool InBool) { bVSync = InBool; }
    
public:
    bool bVSync = true;

private:
    void InitRenderContext();
   
    bool CreateDeviceAndSwapChain(HWND Hwnd);
    bool CreateFrameBuffer();
    bool CreateRasterizerState();
    bool CreateDpethStencilBuffer();
    bool CreateDepthStencilState();
    bool CreateDepthStencillView();
    bool CreateBlendState();
    //bool CreateDefaultSamplerState();

    void ReleaseDeviceAndSwapChain();
    void ReleaseFrameBuffer();
    //void ReleaseRasterizerState();
    void ReleaseDepthStencil();

private:
    bool bIsInitialized = false;

    ID3D11Device* Device = nullptr;
    ID3D11DeviceContext* DeviceContext = nullptr;
    IDXGISwapChain* SwapChain = nullptr;

    ID3D11DepthStencilView* DepthStencilView = nullptr;
    ID3D11Texture2D* DepthStencilBuffer = nullptr;
    ID3D11DepthStencilState* DepthStencilState = nullptr;
    ID3D11BlendState* BlendState = nullptr;
    ID3D11Texture2D* FrameBuffer = nullptr;
    ID3D11RenderTargetView* FrameBufferRTV = nullptr;
    //ID3D11RasterizerState* RasterizerState = nullptr;

    D3D11_VIEWPORT ViewportInfo = D3D11_VIEWPORT();
    FLOAT ClearColor[4] = { 0.025f, 0.025f, 0.025f, 1.0f };

};