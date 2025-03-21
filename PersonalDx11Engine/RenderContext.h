#pragma once
// RenderContext.h
#pragma once
#include <d3d11.h>
#include "Math.h"
#include <stack>
#include <unordered_map>
#include <string>
#include "RenderStateInterface.h"

// 전방 선언
class IRenderJob;

class FRenderContext
{
public:
    FRenderContext() = default;
    ~FRenderContext() = default;

    void SetDeviceContext(ID3D11DeviceContext* InContext) { DeviceContext = InContext; }
    void SetViewMatrix(const Matrix& InView) { ViewMatrix = InView; }
    void SetProjectionMatrix(const Matrix& InProj) { ProjectionMatrix = InProj; }

    const Matrix& GetViewMatrix() const { return ViewMatrix; }
    const Matrix& GetProjectionMatrix() const { return ProjectionMatrix; }
    ID3D11DeviceContext* GetDeviceContext() const { return DeviceContext; }
    ID3D11SamplerState* GetDefaultSamplerState() const { return DefaultSamplerState; }
    void SetDefaultSamplerState(ID3D11SamplerState* InSamplerState) { DefaultSamplerState = InSamplerState; }

    // 렌더 상태 관리
    void PushState(IRenderState* State);
    void PopState();

    // 렌더링 메서드들 - 렌더 잡에서 호출됨
    void BindVertexBuffer(ID3D11Buffer* Buffer, UINT Stride, UINT Offset);
    void BindIndexBuffer(ID3D11Buffer* Buffer, DXGI_FORMAT Format = DXGI_FORMAT_R32_UINT);
    void BindShader(ID3D11VertexShader* VS, ID3D11PixelShader* PS, ID3D11InputLayout* Layout);
    void BindConstantBuffer(UINT Slot, ID3D11Buffer* Buffer, const void* Data, size_t Size, bool IsVertexShader);
    void BindShaderResource(UINT Slot, ID3D11ShaderResourceView* SRV);
    void BindSamplerState(UINT Slot, ID3D11SamplerState* Sampler);
    void Draw(UINT VertexCount, UINT StartVertexLocation = 0);
    void DrawIndexed(UINT IndexCount, UINT StartIndexLocation = 0, INT BaseVertexLocation = 0);

private:

private:
    ID3D11DeviceContext* DeviceContext = nullptr;
    ID3D11SamplerState* DefaultSamplerState = nullptr;
    Matrix ViewMatrix;
    Matrix ProjectionMatrix;

    // 현재 바인딩된 리소스 캐싱
    ID3D11Buffer* CurrentVB = nullptr;
    ID3D11Buffer* CurrentIB = nullptr;
    ID3D11VertexShader* CurrentVS = nullptr;
    ID3D11PixelShader* CurrentPS = nullptr;
    ID3D11InputLayout* CurrentLayout = nullptr;

    std::stack<IRenderState*> StateStack;
};
