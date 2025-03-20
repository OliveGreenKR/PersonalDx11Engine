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

    // 상태 관리
    void PushState(IRenderState* State);
    void PopState();

    // 리소스 바인딩
    void BindVertexBuffer(ID3D11Buffer* Buffer, UINT Stride, UINT Offset);
    void BindIndexBuffer(ID3D11Buffer* Buffer, DXGI_FORMAT Format);

    void BindShader(ID3D11VertexShader* VS, ID3D11PixelShader* PS);

    // 렌더링 명령
    void Draw(UINT VertexCount, UINT StartVertexLocation);
    void DrawIndexed(UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation);

    // 컨텍스트 액세스
    ID3D11DeviceContext* GetDeviceContext() { return DeviceContext; }
};