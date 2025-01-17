#include "Renderer.h"

void URenderer::Initialize(HWND hWindow)
{
    bool result;
    result = RenderHardware->Initialize(hWindow);

    assert(result);

    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    //result = Shader->Initialize(RenderHardware->GetDevice(), L"ShaderW0.hlsl", L"ShaderW0.hlsl", layout, ARRAYSIZE(layout));
    CreateShader();
}

void URenderer::BeforeRender()
{
    RenderHardware->BeginScene();
    Shader->Bind(RenderHardware->GetDeviceContext());
}


void URenderer::EndRender()
{
    RenderHardware->EndScene();
}

void URenderer::Release()
{
    Shader->Release();
    ReleaseShader();
    RenderHardware->Release();
}