#pragma once
#pragma comment(lib, "user32")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")
#pragma comment(lib, "dxguid")

#include <d3d11.h>
#include <d3dcompiler.h>
#include <directxmath.h>

#include <stack>
#include <unordered_map>
#include <string>

#include "RenderState.h"

class FRenderContext
{
private:
    ID3D11DeviceContext* DeviceContext;
    std::stack<FRenderState*> StateStack;
    std::unordered_map<std::string, ID3D11Buffer*> BoundBuffers;

public:
    // ���� ����
    void PushState(FRenderState* State);
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