#pragma once
#include <d3d11.h>
#include <string>
#include <vector>
#include <memory>

class FD3DContextDebugger
{
public:
    struct FResourceBinding
    {
        std::string Type;
        std::string Name;
        UINT Slot;
        void* Resource;
        bool bIsValid;
    };

    FD3DContextDebugger() = default;
    ~FD3DContextDebugger() = default;

    // 컨텍스트의 모든 바인딩 리소스 캡처
    void CaptureBindings(ID3D11DeviceContext* DeviceContext);
    void EnableD3D11DebugMessages(ID3D11Device* device);

    // 캡처된 리소스의 유효성 검사
    bool ValidateAllBindings() const;

    // 디버그 출력 
    void PrintBindings() const;

    // 특정 유형의 리소스에 대한 유효성 검사
    bool ValidateVertexBuffers() const;
    bool ValidateIndexBuffers() const;
    bool ValidateShaders() const;
    bool ValidateConstantBuffers() const;
    bool ValidateShaderResources() const;
    bool ValidateSamplers() const;
    bool ValidateRenderTargets() const;
    bool ValidateRasterizerState() const;
    bool ValidateBlendState() const;
    bool ValidateDepthStencilState() const;

    bool InspectConstantBuffer(ID3D11Device* device, ID3D11DeviceContext* context , UINT maxDisplayBytes = 64);
    bool InspectVertexBuffer(ID3D11Device* device, ID3D11DeviceContext* context, UINT maxDisplayBytes = 64);

private:
    // 버퍼 내용 검사
    bool InspectBufferContent(ID3D11Buffer* buffer, ID3D11Device* device, ID3D11DeviceContext* context , UINT maxDisplayBytes = 64);

    // 캡처된 리소스 저장
    std::vector<FResourceBinding> VertexBuffers;
    std::vector<FResourceBinding> IndexBuffers;
    std::vector<FResourceBinding> ConstantBuffersVS;
    std::vector<FResourceBinding> ConstantBuffersPS;
    std::vector<FResourceBinding> ShaderResources;
    std::vector<FResourceBinding> Samplers;
    FResourceBinding RasterizerState;
    FResourceBinding BlendState;
    FResourceBinding DepthStencilState;
    FResourceBinding RenderTargets[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT];
    FResourceBinding DepthStencilView;
    FResourceBinding VertexShader;
    FResourceBinding PixelShader;
    FResourceBinding InputLayout;

    // 리소스 유효성 검사 (nullptr 체크 및 추가 검증)
    bool ValidateResource(const FResourceBinding& Resource) const;

};