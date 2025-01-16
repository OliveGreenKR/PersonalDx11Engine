#include "Renderer.h"

void URenderer::Initialize(HWND hWindow)
{
    RenderHardware->Initialize(hWindow);
    //CreateShader();
    //CreateConstantBuffer();
}

void URenderer::BeginRender()
{
    RenderHardware->BeginScene();
}

void URenderer::Render()
{
    //Render Somthing..
}

void URenderer::EndRender()
{
    RenderHardware->EndScene();
}

void URenderer::Shutdown()
{
    RenderHardware->Shutdown();
    //ReleaseShader();
}
//
//ID3D11Buffer* URenderer::CreateVertexBuffer(FVertexSimple* vertices, UINT byteWidth)
//{
//    D3D11_BUFFER_DESC vertexbufferdesc = {};
//    vertexbufferdesc.ByteWidth = byteWidth;
//    vertexbufferdesc.Usage = D3D11_USAGE_IMMUTABLE; // will never be updated 
//    vertexbufferdesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
//
//    D3D11_SUBRESOURCE_DATA vertexbufferSRD = { vertices };
//
//    ID3D11Buffer* vertexBuffer;
//
//    Device->CreateBuffer(&vertexbufferdesc, &vertexbufferSRD, &vertexBuffer);
//
//    return vertexBuffer;
//    return nullptr;
//}


//void URenderer::RenderPrimitive(ID3D11Buffer* pBuffer, UINT numVertices)
//{
//    UINT offset = 0;
//    DeviceContext->IASetVertexBuffers(0, 1, &pBuffer, &Stride, &offset);
//
//    DeviceContext->Draw(numVertices, 0);
//}
//
//
//void URenderer::UpdateConstant(FVector InOffset)
//{
//    if (ConstantBuffer)
//    {
//        D3D11_MAPPED_SUBRESOURCE constantbufferMSR;
//
//        DeviceContext->Map(ConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &constantbufferMSR); // update constant buffer every frame
//        FConstants* constants = (FConstants*)constantbufferMSR.pData;
//        {
//            constants->Offset = InOffset;
//        }
//        DeviceContext->Unmap(ConstantBuffer, 0);
//    }
//}
//
//void URenderer::CreateConstantBuffer()
//{
//    D3D11_BUFFER_DESC constantbufferdesc = {};
//    constantbufferdesc.ByteWidth = (sizeof(FConstants) + 0xf) & 0xfffffff0; // ensure constant buffer size is multiple of 16 bytes
//    constantbufferdesc.Usage = D3D11_USAGE_DYNAMIC; // will be updated from CPU every frame
//    constantbufferdesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
//    constantbufferdesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
//
//    Device->CreateBuffer(&constantbufferdesc, nullptr, &ConstantBuffer);
//}
//
//void URenderer::CreateShader()
//{
//    ID3DBlob* vertexshaderCSO;
//    ID3DBlob* pixelshaderCSO;
//
//    D3DCompileFromFile(L"ShaderW0.hlsl", nullptr, nullptr, "mainVS", "vs_5_0", 0, 0, &vertexshaderCSO, nullptr);
//
//    Device->CreateVertexShader(vertexshaderCSO->GetBufferPointer(), vertexshaderCSO->GetBufferSize(), nullptr, &SimpleVertexShader);
//
//    D3DCompileFromFile(L"ShaderW0.hlsl", nullptr, nullptr, "mainPS", "ps_5_0", 0, 0, &pixelshaderCSO, nullptr);
//
//    Device->CreatePixelShader(pixelshaderCSO->GetBufferPointer(), pixelshaderCSO->GetBufferSize(), nullptr, &SimplePixelShader);
//
//    D3D11_INPUT_ELEMENT_DESC layout[] =
//    {
//        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
//        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
//    };
//
//    Device->CreateInputLayout(layout, ARRAYSIZE(layout), vertexshaderCSO->GetBufferPointer(), vertexshaderCSO->GetBufferSize(), &SimpleInputLayout);
//
//
//    Stride = sizeof(FVertexSimple);
//
//    vertexshaderCSO->Release();
//    pixelshaderCSO->Release();
//}
//
//void URenderer::ReleaseShader()
//{
//    if (SimpleInputLayout)
//    {
//        SimpleInputLayout->Release();
//        SimpleInputLayout = nullptr;
//    }
//
//    if (SimplePixelShader)
//    {
//        SimplePixelShader->Release();
//        SimplePixelShader = nullptr;
//    }
//
//    if (SimpleVertexShader)
//    {
//        SimpleVertexShader->Release();
//        SimpleVertexShader = nullptr;
//    }
//}
//
