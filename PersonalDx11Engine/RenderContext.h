﻿#pragma once
#include <d3d11.h>
#include "Math.h"
#include <stack>
#include <memory>
#include "RenderStateInterface.h"
#include "RenderHardwareInterface.h"


class FD3DContextDebugger;

class FRenderContext
{
public:
    FRenderContext() = default;
    ~FRenderContext() = default;

    bool Initialize(std::shared_ptr<IRenderHardware> InHardware);
    void Release();

    // 렌더 상태 관리
    void PushState(IRenderState* State);
    void PopState();

    //프레임 컨텍스트 관리
    void BeginFrame();
    void EndFrame();

    // 쉐이더 
    void BindShader(ID3D11VertexShader* VS, ID3D11PixelShader* PS, ID3D11InputLayout* Layout);

    // 렌더링 리소스  
    void BindVertexBuffer(ID3D11Buffer* Buffer, UINT Stride, UINT Offset);
    void BindIndexBuffer(ID3D11Buffer* Buffer, DXGI_FORMAT Format = DXGI_FORMAT_R32_UINT);
    void BindConstantBuffer(UINT Slot, ID3D11Buffer* Buffer, const void* Data, size_t Size, bool IsVertexShader);
    void BindShaderResource(UINT Slot, ID3D11ShaderResourceView* SRV);
    void BindSamplerState(UINT Slot, ID3D11SamplerState* Sampler);
    void Draw(UINT VertexCount, UINT StartVertexLocation = 0);
    void DrawIndexed(UINT IndexCount, UINT StartIndexLocation = 0, INT BaseVertexLocation = 0);

    ID3D11SamplerState* GetDefaultSamplerState() { return DefaultSamplerState; }
    
    //접근자
    ID3D11DeviceContext* GetDeviceContext() const { return RenderHardware ? RenderHardware->GetDeviceContext() : nullptr; }
    ID3D11Device* GetDevice() const { return RenderHardware ? RenderHardware->GetDevice() : nullptr; }

    // 디버그 메소드
    void ValidateDeviceContextBindings();
public:
    //디버그 설정
    bool bDebugValidationEnabled = false;
    bool bDebugBreakOnError = false;
 
private:
    bool CreateDefaultSamplerState();

private:
    // 현재 바인딩된 리소스 캐시
    ID3D11Buffer* CurrentVB = nullptr;
    ID3D11Buffer* CurrentIB = nullptr;

    //쉐이더 리소스
    ID3D11VertexShader* CurrentVS = nullptr;
    ID3D11PixelShader* CurrentPS = nullptr;
    ID3D11InputLayout* CurrentLayout = nullptr;
    
    //기본 렌더링 상태
    ID3D11SamplerState* DefaultSamplerState = nullptr;

    std::shared_ptr<IRenderHardware> RenderHardware;
    std::stack<IRenderState*> StateStack;


#ifdef _DEBUG
    class FD3DContextDebugger* ContextDebugger;
#endif
};