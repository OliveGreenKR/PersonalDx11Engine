#pragma once
#include <stack>
#include <unordered_map>
#include <map>
#include <string>
#include <array>

#include "RenderStateInterface.h"

class FRenderContext
{
private:
    ID3D11DeviceContext* DeviceContext;
    std::stack<IRenderState*> StateStack;

    std::unordered_map<std::string, ID3D11Buffer*> BoundBuffers;
    std::unordered_map<std::string, ID3D11VertexShader*> VertexShaders;
    std::unordered_map<std::string, ID3D11PixelShader*> PixelShaders;

    ID3D11VertexShader* CurrentVS;
    ID3D11PixelShader* CurrentPS;
    ID3D11InputLayout* CurrentInputLayout = nullptr;

    ID3D11SamplerState* SamplerState = nullptr;

     // 각 슬롯에 현재 바인딩된 상수 버퍼
    std::array<ID3D11Buffer*, 16> VSBoundConstantBuffers = {};
    std::array<ID3D11Buffer*, 16> PSBoundConstantBuffers = {};

public:
    void SetDeviceContext(ID3D11DeviceContext* InContext) 
	{
		if (InContext)
		{
            DeviceContext = InContext;
		}
	}

    // 상태 관리
    void PushState(IRenderState* State);
    void PopState();

    // 상수 버퍼 바인딩 
    void BindVSConstantBuffer(UINT Slot, ID3D11Buffer* Buffer);
    void BindPSConstantBuffer(UINT Slot, ID3D11Buffer* Buffer);

    // 상수 버퍼 데이터 업데이트 - DeviceContext 접근과 Map/Unmap 담당
    void UpdateVSConstantBuffer(UINT StartSlot, ID3D11Buffer* Buffer, const void* Data, size_t DataSize);
    void UpdatePSConstantBuffer(UINT StartSlot, ID3D11Buffer* Buffer, const void* Data, size_t DataSize);

    // 리소스 바인딩
    void BindVertexBuffer(ID3D11Buffer* Buffer, UINT Stride, UINT Offset);
    void BindIndexBuffer(ID3D11Buffer* Buffer, DXGI_FORMAT Format = DXGI_FORMAT_R32_UINT);
    void BindShader(ID3D11VertexShader* VS, ID3D11PixelShader* PS);


    // 3. 텍스처 및 샘플러 바인딩
    void BindShaderResource(UINT Slot, ID3D11ShaderResourceView* Texture);
    void BindSampler(UINT Slot, ID3D11SamplerState* SamplerState);

    // 렌더링 명령
    void Draw(UINT VertexCount, UINT StartVertexLocation);
    void DrawIndexed(UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation);

    // 컨텍스트 액세스
    ID3D11DeviceContext* GetDeviceContext() { return DeviceContext; }
};