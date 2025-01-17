#pragma once
#include "D3D.h"
#include "D3DShader.h"
//ImGui
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_impl_dx11.h"
#include "imGui/imgui_impl_win32.h"

struct FVertexSimple
{
	float x, y, z;    // Position
	float r, g, b, a; // Color
};

class URenderer 
{

public:
    URenderer() = default;
    ~URenderer() = default;

public:
    void Initialize(HWND hWindow);

    void BeforeRender();

    void EndRender();

    void Release();

public:
    //tmp
    __forceinline ID3D11Device* GetDevice() { return RenderHardware->GetDevice(); }
    __forceinline ID3D11DeviceContext* GetDeviceContext() { return RenderHardware->GetDeviceContext(); }

public:
    __forceinline void SetVSync(bool activation) { RenderHardware->bVSync = activation; }


public:
    //void RenderPrimitive(ID3D11Buffer* pBuffer, UINT numVertices);
    //void UpdateConstant(FVector InOffset);
#pragma region shader test
    ID3D11VertexShader* SimpleVertexShader;
    ID3D11PixelShader* SimplePixelShader;
    ID3D11InputLayout* SimpleInputLayout;
    unsigned int Stride;

	void CreateShader()
	{
		ID3DBlob* vertexshaderCSO;
		ID3DBlob* pixelshaderCSO;

		D3DCompileFromFile(L"ShaderW0.hlsl", nullptr, nullptr, "mainVS", "vs_5_0", 0, 0, &vertexshaderCSO, nullptr);

		GetDevice()->CreateVertexShader(vertexshaderCSO->GetBufferPointer(), vertexshaderCSO->GetBufferSize(), nullptr, &SimpleVertexShader);

		D3DCompileFromFile(L"ShaderW0.hlsl", nullptr, nullptr, "mainPS", "ps_5_0", 0, 0, &pixelshaderCSO, nullptr);

		GetDevice()->CreatePixelShader(pixelshaderCSO->GetBufferPointer(), pixelshaderCSO->GetBufferSize(), nullptr, &SimplePixelShader);

		D3D11_INPUT_ELEMENT_DESC layout[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		HRESULT hr = GetDevice()->CreateInputLayout(layout, ARRAYSIZE(layout), vertexshaderCSO->GetBufferPointer(), vertexshaderCSO->GetBufferSize(), &SimpleInputLayout);
		assert(SUCCEEDED(hr));

		Stride = sizeof(FVertexSimple);

		vertexshaderCSO->Release();
		pixelshaderCSO->Release();
	}

	void ReleaseShader()
	{
		if (SimpleInputLayout)
		{
			SimpleInputLayout->Release();
			SimpleInputLayout = nullptr;
		}

		if (SimplePixelShader)
		{
			SimplePixelShader->Release();
			SimplePixelShader = nullptr;
		}

		if (SimpleVertexShader)
		{
			SimpleVertexShader->Release();
			SimpleVertexShader = nullptr;
		}
	}


	void PrepareShader()
	{
		GetDeviceContext()->VSSetShader(SimpleVertexShader, nullptr, 0);
		GetDeviceContext()->PSSetShader(SimplePixelShader, nullptr, 0);
		GetDeviceContext()->IASetInputLayout(SimpleInputLayout);
	}

	void RenderPrimitive(ID3D11Buffer* pBuffer, UINT numVertices)
	{
		UINT offset = 0;
		GetDeviceContext()->IASetVertexBuffers(0, 1, &pBuffer, &Stride, &offset);
		GetDeviceContext()->Draw(numVertices, 0);
	}

#pragma endregion

private:
    FD3D* RenderHardware = new FD3D();
    FD3DShader* Shader = new FD3DShader();

};
