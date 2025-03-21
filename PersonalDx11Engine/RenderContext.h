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

     // �� ���Կ� ���� ���ε��� ��� ����
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

    // ���� ����
    void PushState(IRenderState* State);
    void PopState();

    // ��� ���� ���ε� 
    void BindVSConstantBuffer(UINT Slot, ID3D11Buffer* Buffer);
    void BindPSConstantBuffer(UINT Slot, ID3D11Buffer* Buffer);

    // ��� ���� ������ ������Ʈ - DeviceContext ���ٰ� Map/Unmap ���
    void UpdateVSConstantBuffer(UINT StartSlot, ID3D11Buffer* Buffer, const void* Data, size_t DataSize);
    void UpdatePSConstantBuffer(UINT StartSlot, ID3D11Buffer* Buffer, const void* Data, size_t DataSize);

    // ���ҽ� ���ε�
    void BindVertexBuffer(ID3D11Buffer* Buffer, UINT Stride, UINT Offset);
    void BindIndexBuffer(ID3D11Buffer* Buffer, DXGI_FORMAT Format = DXGI_FORMAT_R32_UINT);
    void BindShader(ID3D11VertexShader* VS, ID3D11PixelShader* PS);


    // 3. �ؽ�ó �� ���÷� ���ε�
    void BindShaderResource(UINT Slot, ID3D11ShaderResourceView* Texture);
    void BindSampler(UINT Slot, ID3D11SamplerState* SamplerState);

    // ������ ���
    void Draw(UINT VertexCount, UINT StartVertexLocation);
    void DrawIndexed(UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation);

    // ���ؽ�Ʈ �׼���
    ID3D11DeviceContext* GetDeviceContext() { return DeviceContext; }
};