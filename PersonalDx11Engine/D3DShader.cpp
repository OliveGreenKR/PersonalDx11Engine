#include "D3DShader.h"
#include "d3d11shader.h"

FD3DShader::~FD3DShader()
{
	Shutdown();
}

bool FD3DShader::Initialize(ID3D11Device* Device, const wchar_t* vertexShaderPath, const wchar_t* pixelShaderPath, D3D11_INPUT_ELEMENT_DESC* layout, unsigned int layoutSize)
{
	HRESULT result;

	//Create Shaders
	ID3DBlob* VertexShaderBlob;
	ID3DBlob* PixelShaderBlob;

	result = D3DCompileFromFile(vertexShaderPath, nullptr, nullptr, "mainVS", "vs_5_0", 0, 0, &VertexShaderBlob, nullptr);
	if (FAILED(result))
		return false;

	result = D3DCompileFromFile(pixelShaderPath, nullptr, nullptr, "mainPS", "ps_5_0", 0, 0, &PixelShaderBlob, nullptr);
	if (FAILED(result))
		return false;

	result = Device->CreateVertexShader(VertexShaderBlob->GetBufferPointer(), VertexShaderBlob->GetBufferSize(), nullptr, &VertexShader);
	if (FAILED(result))
		return false;

	result = Device->CreatePixelShader(PixelShaderBlob->GetBufferPointer(), PixelShaderBlob->GetBufferSize(), nullptr, &PixelShader);
	if (FAILED(result))
		return false;

	//Creatr InputLayout
	result = Device->CreateInputLayout(layout, layoutSize, VertexShaderBlob->GetBufferPointer(), VertexShaderBlob->GetBufferSize(), &InputLayout);
	if (FAILED(result))
		return false;


	//Create ConstantBuffer
	ID3D11ShaderReflection* Reflector = nullptr;
	result = D3DReflect(VertexShaderBlob->GetBufferPointer(),VertexShaderBlob->GetBufferSize(),IID_ID3D11ShaderReflection,(void**)&Reflector);

	if (SUCCEEDED(result))
	{
		D3D11_SHADER_DESC ShaderDesc;
		Reflector->GetDesc(&ShaderDesc);

		// 상수 버퍼 생성
		for (unsigned int i = 0; i < ShaderDesc.ConstantBuffers; i++)
		{
			ID3D11ShaderReflectionConstantBuffer* CBReflection =
				Reflector->GetConstantBufferByIndex(i);

			D3D11_SHADER_BUFFER_DESC BufferDesc;
			CBReflection->GetDesc(&BufferDesc);

			D3D11_BUFFER_DESC ConstantBufferDesc = {};
			ConstantBufferDesc.ByteWidth = BufferDesc.Size;
			ConstantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
			ConstantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			ConstantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			ConstantBufferDesc.MiscFlags = 0;
			ConstantBufferDesc.StructureByteStride = 0;

			ID3D11Buffer* ConstantBuffer = nullptr;
			result = Device->CreateBuffer(&ConstantBufferDesc, nullptr, &ConstantBuffer);
			if (FAILED(result))
				return false;
			else
			{
				ConstantBuffers.push_back(ConstantBuffer);
			}
		}
		Reflector->Release();
	}

	VertexShaderBlob->Release();
	PixelShaderBlob->Release();

	return true;
}

void FD3DShader::Shutdown()
{
	for (ID3D11Buffer* Buffer : ConstantBuffers)
	{
		if (Buffer)
		{
			Buffer->Release();
		}
	}
	ConstantBuffers.clear();

	if (InputLayout)
	{
		InputLayout->Release();
		InputLayout = nullptr;
	}

	if (PixelShader)
	{
		PixelShader->Release();
		PixelShader = nullptr;
	}

	if (VertexShader)
	{
		VertexShader->Release();
		VertexShader = nullptr;
	}

}

void FD3DShader::Bind(ID3D11DeviceContext* DeviceContext)
{
	DeviceContext->VSSetShader(VertexShader, nullptr, 0);
	DeviceContext->PSSetShader(PixelShader, nullptr, 0);
	DeviceContext->IASetInputLayout(InputLayout);
}

void FD3DShader::SetTexture(ID3D11DeviceContext* DeviceContext, ID3D11ShaderResourceView* TextureView)
{
}

void FD3DShader::SetSamplerState(ID3D11DeviceContext* DeviceContext, ID3D11SamplerState* SamplerState)
{
}

