#pragma once
#include "D3D.h";
#include "D3DShader.h"
//ImGui
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_impl_dx11.h"
#include "imGui/imgui_impl_win32.h"

#include "Vector.h"


struct FVertexSimple
{
    float x, y, z;    // Position
    float r, g, b, a; // Color
};

class URenderer 
{
    struct FConstants
    {
        FVector Offset;
        float pad;
    };

public:
    URenderer() = default;
    ~URenderer() = default;

public:
    void Initialize(HWND hWindow);

    void BeginRender();
    void Render();
    void EndRender();

    void Shutdown();

public:
    __forceinline void SetVSync(bool activation) { RenderHardware->bVSync = activation; }


public:
    //ID3D11Buffer* CreateVertexBuffer(FVertexSimple* vertices, UINT byteWidth);
    //void ReleaseVertexBuffer(ID3D11Buffer* vertexBuffer) { vertexBuffer->Release(); }

    void RenderPrimitive(ID3D11Buffer* pBuffer, UINT numVertices);
    void UpdateConstant(FVector InOffset);

private:
    void CreateConstantBuffer();


private:
    FD3D* RenderHardware = new FD3D();
    FD3DShader* Shader = new FD3DShader();


#pragma region shader
private:
    //todo: shader 인터페이스로 기능 분리?
    //ID3D11VertexShader* SimpleVertexShader;
    //ID3D11PixelShader* SimplePixelShader;
    //ID3D11InputLayout* SimpleInputLayout;
    //unsigned int Stride;

    //void CreateShader();
    //void ReleaseShader();
    //ID3D11Buffer* ConstantBuffer = nullptr; // 쉐이더에 데이터를 전달하기 위한 상수 버퍼
#pragma endregion

    FLOAT ClearColor[4] = { 0.025f, 0.025f, 0.025f, 1.0f }; // 화면을 초기화(clear)할 때 사용할 색상 (RGBA)
    D3D11_VIEWPORT ViewportInfo; // 렌더링 영역을 정의하는 뷰포트 정보

};
