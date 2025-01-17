#include "D3DShader.h"
#include "d3d11shader.h"


FD3DShader::~FD3DShader()
{
	Release();
}

bool FD3DShader::Initialize(ID3D11Device* Device, const wchar_t* vertexShaderPath, const wchar_t* pixelShaderPath, D3D11_INPUT_ELEMENT_DESC* layout, const unsigned int layoutSize)
{
	HRESULT result;

	//Create Shaders
	ID3DBlob* VSBlob;
	ID3DBlob* PSBlob;

	result = D3DCompileFromFile(vertexShaderPath, nullptr, nullptr, "mainVS", "vs_5_0", 0, 0, &VSBlob, nullptr);
	if (FAILED(result))
	{
		return false;
	}

	result = D3DCompileFromFile(pixelShaderPath, nullptr, nullptr, "mainPS", "ps_5_0", 0, 0, &PSBlob, nullptr);
	if (FAILED(result))
	{
		return false;
	}

	result = Device->CreateVertexShader(VSBlob->GetBufferPointer(), VSBlob->GetBufferSize(), nullptr, &VertexShader);
	if (FAILED(result))
	{
		return false;
	}

	result = Device->CreatePixelShader(PSBlob->GetBufferPointer(), PSBlob->GetBufferSize(), nullptr, &PixelShader);
	if (FAILED(result))
	{
		return false;
	}


	//Creatr InputLayout
	result = Device->CreateInputLayout(layout, layoutSize, VSBlob->GetBufferPointer(), VSBlob->GetBufferSize(), &InputLayout);
	if (FAILED(result))
	{
		return false;
	}
		

	//Create ConstantBuffer
	ID3D11ShaderReflection* Reflector = nullptr;
	result = D3DReflect(VSBlob->GetBufferPointer(), VSBlob->GetBufferSize(), IID_ID3D11ShaderReflection, (void**)&Reflector);

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
			if(SUCCEEDED(result))
			{
				ConstantBuffers.push_back(ConstantBuffer);
			}
		}
		Reflector->Release();
	}


	VSBlob->Release();
	PSBlob->Release();

	return true;
}

void FD3DShader::Release()
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

	for (auto constantBuffer : ConstantBuffers)
	{
		DeviceContext->VSSetConstantBuffers(0, 1, &constantBuffer);
	}
}
