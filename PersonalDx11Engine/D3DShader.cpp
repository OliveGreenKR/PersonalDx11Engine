#include "D3DShader.h"
#include "d3d11shader.h"
#include "Debug.h"


UShader::~UShader()
{
	Release();
}

void UShader::Load(ID3D11Device* Device, const wchar_t* vertexShaderPath, const wchar_t* pixelShaderPath, D3D11_INPUT_ELEMENT_DESC* layout, const unsigned int layoutSize)
{
	HRESULT result;

	//Create Shaders
	ID3DBlob* VSBlob;
	ID3DBlob* PSBlob;
	ID3DBlob* errorBlob = nullptr;

	result = CompileShader(vertexShaderPath,"mainVS", "vs_5_0",&VSBlob);
	assert(SUCCEEDED(result),"vertexvShader compile failed.");
	result = CompileShader(pixelShaderPath, "mainPS", "ps_5_0", &PSBlob);
	assert(SUCCEEDED(result), "pixel Shader compile failed.");

	result = Device->CreateVertexShader(VSBlob->GetBufferPointer(), VSBlob->GetBufferSize(), nullptr, &VertexShader);
	assert(SUCCEEDED(result), "vetex Shader create failed");

	result = Device->CreatePixelShader(PSBlob->GetBufferPointer(), PSBlob->GetBufferSize(), nullptr, &PixelShader);
	assert(SUCCEEDED(result), "pixel Shader create failed.");

	//Creatr InputLayout
	result = Device->CreateInputLayout(layout, layoutSize, VSBlob->GetBufferPointer(), VSBlob->GetBufferSize(), &InputLayout);
	assert(SUCCEEDED(result), "input layout create failed.");
		

	//Create ConstantBuffer - Only vertex reference
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

	//VSBlob->Release();
	// VSBlob 저장
	VSByteCode = VSBlob;
	PSBlob->Release();

	// 메모리 사용량 계산
	CalculateMemoryUsage();

	// 로드 상태 업데이트
	bIsLoaded = true;
}

void UShader::Release()
{
	if (SamplerState)
	{
		SamplerState->Release();
		SamplerState = nullptr;
	}
	
	if (VSByteCode)
	{
		VSByteCode->Release();
		VSByteCode = nullptr;
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

	/// 메모리 사용량 초기화
	MemorySize = 0;

	// 로드 상태 업데이트
	bIsLoaded = false;
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
	if (!DeviceContext)
	{
		return;
	}

	UINT SlotIndex = static_cast<UINT>(Slot);
	DeviceContext->PSSetShaderResources(SlotIndex, 1, &Texture);
}

void UShader::BindMatrix(ID3D11DeviceContext* DeviceContext, FMatrixBufferData& BufferData)
{
	BufferData.World = XMMatrixTranspose(BufferData.World);
	BufferData.View = XMMatrixTranspose(BufferData.View);
	BufferData.Projection = XMMatrixTranspose(BufferData.Projection);
	UpdateConstantBuffer<FMatrixBufferData>(DeviceContext, BufferData, EBufferSlot::Matrix);
}

void UShader::BindColor(ID3D11DeviceContext* DeviceContext, FColorBufferData& BufferData)
{
	UpdateConstantBuffer<FColorBufferData>(DeviceContext, BufferData, EBufferSlot::Color);
}

void UShader::GetShaderBytecode(const void** bytecode, size_t* length) const
{
	// 컴파일된 셰이더 바이트코드 저장 필요
		if (VSByteCode) {
			*bytecode = VSByteCode->GetBufferPointer();
			*length = VSByteCode->GetBufferSize();
		}
		else {
			*bytecode = nullptr;
			*length = 0;
		}
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

HRESULT UShader::CompileShader(const wchar_t* filename, const char* entryPoint, const char* target, ID3DBlob** ppBlob)
{
	ID3DBlob* pBlob = nullptr;
	ID3DBlob* pErrorBlob = nullptr;

	HRESULT hr = D3DCompileFromFile(filename, nullptr, nullptr, entryPoint, target, 0, 0, &pBlob, &pErrorBlob);
	if (FAILED(hr))
	{
		if (pErrorBlob)
		{
			OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
			pErrorBlob->Release();
		}
		*ppBlob = nullptr; // 실패 시 nullptr 반환
		return hr;
	}

	*ppBlob = pBlob;
	return S_OK;
}

// 메모리 사용량 계산 메서드 추가
void UShader::CalculateMemoryUsage()
{
	MemorySize = 0;

	// VSByteCode 크기
	if (VSByteCode)
	{
		MemorySize += VSByteCode->GetBufferSize();
	}

	// ConstantBuffers 크기 추가
	for (auto* buffer : ConstantBuffers)
	{
		if (buffer)
		{
			D3D11_BUFFER_DESC desc;
			buffer->GetDesc(&desc);
			MemorySize += desc.ByteWidth;
		}
	}
	
	// 기타 D3D 객체의 추정 메모리 추가
	// 실제 메모리 사용량은 드라이버와 하드웨어에 따라 달라질 수 있으므로 대략적인 추정치 사용
	if (VertexShader)MemorySize += 128;    // 추정치
	if (PixelShader) MemorySize += 128;     // 추정치
	if (InputLayout) MemorySize += 64;      // 추정치
	if (SamplerState) MemorySize += 32;     // 추정치
}
