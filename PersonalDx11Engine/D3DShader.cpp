#include "D3DShader.h"
#include "d3d11shader.h"


UShader::~UShader()
{
	Release();
}

void UShader::Initialize(ID3D11Device* Device, const wchar_t* vertexShaderPath, const wchar_t* pixelShaderPath, D3D11_INPUT_ELEMENT_DESC* layout, const unsigned int layoutSize)
{
	HRESULT result;

	//Create Shaders
	ID3DBlob* VSBlob;
	ID3DBlob* PSBlob;

	result = D3DCompileFromFile(vertexShaderPath, nullptr, nullptr, "mainVS", "vs_5_0", 0, 0, &VSBlob, nullptr);
	assert(SUCCEEDED(result),"vertexvShader compile failed.");
	result = D3DCompileFromFile(pixelShaderPath, nullptr, nullptr, "mainPS", "ps_5_0", 0, 0, &PSBlob, nullptr);
	assert(SUCCEEDED(result), "pixel Shader compile failed.");

	result = Device->CreateVertexShader(VSBlob->GetBufferPointer(), VSBlob->GetBufferSize(), nullptr, &VertexShader);
	assert(SUCCEEDED(result), "vetex Shader create failed");

	result = Device->CreatePixelShader(PSBlob->GetBufferPointer(), PSBlob->GetBufferSize(), nullptr, &PixelShader);
	assert(SUCCEEDED(result), "pixel Shader create failed.");

	//Creatr InputLayout
	result = Device->CreateInputLayout(layout, layoutSize, VSBlob->GetBufferPointer(), VSBlob->GetBufferSize(), &InputLayout);
	assert(SUCCEEDED(result), "input layout create failed.");
		

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
			assert(SUCCEEDED(result), "Constatnt BUffer [%d] create failed.", i);
			if(SUCCEEDED(result))
			{
				ConstantBuffers.push_back(ConstantBuffer);
			}
		}
		Reflector->Release();
	}


	VSBlob->Release();
	PSBlob->Release();
}

void UShader::Release()
{
	if (SamplerState)
	{
		SamplerState->Release();
		SamplerState = nullptr;
	}

	for (ID3D11Buffer* Buffer : ConstantBuffers)
	{
		if (Buffer)
		{
			Buffer->Release();
			Buffer = nullptr;
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

void UShader::Bind(ID3D11DeviceContext* DeviceContext, ID3D11SamplerState* InSamplerState)
{
	DeviceContext->VSSetShader(VertexShader, nullptr, 0);
	DeviceContext->PSSetShader(PixelShader, nullptr, 0);
	DeviceContext->IASetInputLayout(InputLayout);

	if (InSamplerState)
	{
		SetSamplerState(InSamplerState);
	}

	if(SamplerState)
	{
		DeviceContext->PSSetSamplers(0, 1, &SamplerState);
	}
}

void UShader::BindTexture(ID3D11DeviceContext* DeviceContext, ID3D11ShaderResourceView* Texture, ETextureSlot Slot)
{
	if (!DeviceContext || !Texture)
	{
		return;
	}

	if (Texture)
	{
		UINT SlotIndex = static_cast<UINT>(Slot);
		DeviceContext->PSSetShaderResources(SlotIndex, 1, &Texture);
	}
}

void UShader::BindMatrix(ID3D11DeviceContext* DeviceContext, FMatrixBufferData& BufferData)
{
	BufferData.World = XMMatrixTranspose(BufferData.World);
	BufferData.View = XMMatrixTranspose(BufferData.View);
	BufferData.Projection = XMMatrixTranspose(BufferData.Projection);
	UpdateConstantBuffer<FMatrixBufferData>(DeviceContext, BufferData, EBufferSlot::Matrix);
}

void UShader::SetSamplerState(ID3D11SamplerState* InSamplerState)
{
	// 새로운 샘플러 설정
		//COM객체이므로 참조 카운트 관리 필요
	if (SamplerState)
	{
		SamplerState->Release();
	}
	SamplerState = InSamplerState;
	if (InSamplerState)
	{
		SamplerState->AddRef();
	}
}
