#pragma once
#include <stack>
#include <unordered_map>
#include <map>
#include <string>

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

    // ���ҽ� ���ε�
    void BindVertexBuffer(ID3D11Buffer* Buffer, UINT Stride, UINT Offset);
    void BindIndexBuffer(ID3D11Buffer* Buffer, DXGI_FORMAT Format);

    void BindShader(ID3D11VertexShader* VS, ID3D11PixelShader* PS);

    // ������ ���
    void Draw(UINT VertexCount, UINT StartVertexLocation);
    void DrawIndexed(UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation);

    // ���ؽ�Ʈ �׼���
    ID3D11DeviceContext* GetDeviceContext() { return DeviceContext; }
};