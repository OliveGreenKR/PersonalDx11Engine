#include "D3DShader.h"
#include "d3d11shader.h"
#include "Debug.h"
#include <map>


bool UShader::Load(ID3D11Device* Device, const wchar_t* VSPath, const wchar_t* PSPath)
{
	// 1. 쉐이더 컴파일
	ID3DBlob* VSBlob = nullptr;
	ID3DBlob* PSBlob = nullptr;

	if (FAILED(CompileShader(VSPath, "mainVS", "vs_5_0", &VSBlob)) ||
		FAILED(CompileShader(PSPath, "mainPS", "ps_5_0", &PSBlob)) ||
		!PSBlob || !VSBlob)
	{
		return false;
	}

	//ID3DBlob* disasmBlob = nullptr;
	//D3DDisassemble(PSBlob->GetBufferPointer(), PSBlob->GetBufferSize(), 0, nullptr, &disasmBlob);
	//if (disasmBlob)
	//{
	//	LOG("Disassembled PS:\n%s\n", (char*)disasmBlob->GetBufferPointer());
	//	disasmBlob->Release();
	//}

	// 2. 리플렉션 객체 생성
	ID3D11ShaderReflection* VSReflection = nullptr;
	if (FAILED(D3DReflect(VSBlob->GetBufferPointer(), VSBlob->GetBufferSize(),
						  IID_ID3D11ShaderReflection, (void**)&VSReflection)))
	{
		VSBlob->Release();
		PSBlob->Release();
		return false;
	}

	// 3. 입력 레이아웃 자동 생성
	std::vector<D3D11_INPUT_ELEMENT_DESC> InputLayoutDesc;
	if (!CreateInputLayoutFromReflection(VSReflection, InputLayoutDesc))
	{
		VSReflection->Release();
		VSBlob->Release();
		PSBlob->Release();
		return false;
	}

	// 4. 쉐이더 객체 생성
	if (FAILED(Device->CreateVertexShader(VSBlob->GetBufferPointer(), VSBlob->GetBufferSize(),
										  nullptr, &VertexShader)) ||
		FAILED(Device->CreatePixelShader(PSBlob->GetBufferPointer(), PSBlob->GetBufferSize(),
										 nullptr, &PixelShader)) ||
		FAILED(Device->CreateInputLayout(InputLayoutDesc.data(), InputLayoutDesc.size(),
										 VSBlob->GetBufferPointer(), VSBlob->GetBufferSize(), &InputLayout)))
	{
		VSReflection->Release();
		VSBlob->Release();
		PSBlob->Release();
		return false;
	}

	// 5. 상수 버퍼 정보 추출 및 생성
	ExtractAndCreateConstantBuffers(Device, VSReflection, VSConstantBuffers);

	// 픽셀 쉐이더 리플렉션도 동일하게 처리
	ID3D11ShaderReflection* PSReflection = nullptr;
	if (SUCCEEDED(D3DReflect(PSBlob->GetBufferPointer(), PSBlob->GetBufferSize(),
							 IID_ID3D11ShaderReflection, (void**)&PSReflection)))
	{
		ExtractAndCreateConstantBuffers(Device, PSReflection, PSConstantBuffers);
	}

	// 6. 쉐이더 리소스 및 샘플러 정보 추출
	ExtractResourceBindings(VSReflection, VSResourceBindingMeta);
	if (PSReflection)
	{
		ExtractResourceBindings(PSReflection, PSResourceBindingMeta);
	}
		
	// 리소스 정리
	VSReflection->Release();
	PSReflection->Release();
	VSByteCode = VSBlob; // 나중에 쓸 수 있으므로 보관
	PSBlob->Release();

	bIsLoaded = true;
	return true;
}

UShader::~UShader()
{
	Release();
}

void UShader::Release()
{

	if (VSByteCode)
	{
		VSByteCode->Release();
		VSByteCode = nullptr;
	}

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

bool UShader::CreateInputLayoutFromReflection(ID3D11ShaderReflection* Reflection, std::vector<D3D11_INPUT_ELEMENT_DESC>& OutLayout)
{
	D3D11_SHADER_DESC ShaderDesc;
	Reflection->GetDesc(&ShaderDesc);

	// 시맨틱 이름 중복 방지를 위한 카운터
	std::map<std::string, UINT> SemanticIndexMap;

	for (UINT i = 0; i < ShaderDesc.InputParameters; i++)
	{
		D3D11_SIGNATURE_PARAMETER_DESC ParamDesc;
		Reflection->GetInputParameterDesc(i, &ParamDesc);

		// 시맨틱 인덱스 결정
		std::string SemanticName = ParamDesc.SemanticName;
		UINT SemanticIndex = 0;

		if (SemanticIndexMap.find(SemanticName) != SemanticIndexMap.end())
			SemanticIndex = SemanticIndexMap[SemanticName]++;
		else
			SemanticIndexMap[SemanticName] = 1;

		// DXGI 포맷 결정
		DXGI_FORMAT Format = DXGI_FORMAT_UNKNOWN;
		if (ParamDesc.Mask == 1) // 단일 컴포넌트
		{
			switch (ParamDesc.ComponentType)
			{
				case D3D_REGISTER_COMPONENT_FLOAT32: Format = DXGI_FORMAT_R32_FLOAT; break;
				case D3D_REGISTER_COMPONENT_SINT32: Format = DXGI_FORMAT_R32_SINT; break;
				case D3D_REGISTER_COMPONENT_UINT32: Format = DXGI_FORMAT_R32_UINT; break;
			}
		}
		else if (ParamDesc.Mask <= 3) // 2 컴포넌트
		{
			switch (ParamDesc.ComponentType)
			{
				case D3D_REGISTER_COMPONENT_FLOAT32: Format = DXGI_FORMAT_R32G32_FLOAT; break;
				case D3D_REGISTER_COMPONENT_SINT32: Format = DXGI_FORMAT_R32G32_SINT; break;
				case D3D_REGISTER_COMPONENT_UINT32: Format = DXGI_FORMAT_R32G32_UINT; break;
			}
		}
		else if (ParamDesc.Mask <= 7) // 3 컴포넌트
		{
			switch (ParamDesc.ComponentType)
			{
				case D3D_REGISTER_COMPONENT_FLOAT32: Format = DXGI_FORMAT_R32G32B32_FLOAT; break;
				case D3D_REGISTER_COMPONENT_SINT32: Format = DXGI_FORMAT_R32G32B32_SINT; break;
				case D3D_REGISTER_COMPONENT_UINT32: Format = DXGI_FORMAT_R32G32B32_UINT; break;
			}
		}
		else if (ParamDesc.Mask <= 15) // 4 컴포넌트
		{
			switch (ParamDesc.ComponentType)
			{
				case D3D_REGISTER_COMPONENT_FLOAT32: Format = DXGI_FORMAT_R32G32B32A32_FLOAT; break;
				case D3D_REGISTER_COMPONENT_SINT32: Format = DXGI_FORMAT_R32G32B32A32_SINT; break;
				case D3D_REGISTER_COMPONENT_UINT32: Format = DXGI_FORMAT_R32G32B32A32_UINT; break;
			}
		}

		if (Format == DXGI_FORMAT_UNKNOWN)
			continue; // 지원되지 않는 포맷

		// 입력 레이아웃 요소 생성
		D3D11_INPUT_ELEMENT_DESC ElementDesc;
		ElementDesc.SemanticName = _strdup(ParamDesc.SemanticName); // 주의: 메모리 해제 필요
		ElementDesc.SemanticIndex = SemanticIndex;
		ElementDesc.Format = Format;
		ElementDesc.InputSlot = 0;
		ElementDesc.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
		ElementDesc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		ElementDesc.InstanceDataStepRate = 0;

		OutLayout.push_back(ElementDesc);
	}

	return !OutLayout.empty();
}

// 상수 버퍼 정보 추출 및 생성
void UShader::ExtractAndCreateConstantBuffers(ID3D11Device* Device, ID3D11ShaderReflection* Reflection, std::vector<FInternalConstantBufferInfo>& OutBuffers)
{
	D3D11_SHADER_DESC ShaderDesc; 
	Reflection->GetDesc(&ShaderDesc);

	for (UINT i = 0; i < ShaderDesc.ConstantBuffers; i++)
	{
		ID3D11ShaderReflectionConstantBuffer* CBReflection = Reflection->GetConstantBufferByIndex(i);
		D3D11_SHADER_BUFFER_DESC BufferDesc;
		CBReflection->GetDesc(&BufferDesc);

		FInternalConstantBufferInfo BufferInfo;
		BufferInfo.Name = BufferDesc.Name;
		BufferInfo.Size = BufferDesc.Size;
		BufferInfo.BindPoint = i; // 쉐이더에서의 바인딩 포인트 (b0, b1, ...)

		// 상수 버퍼의 변수 정보 추출
		for (UINT j = 0; j < BufferDesc.Variables; j++)
		{
			ID3D11ShaderReflectionVariable* VarReflection = CBReflection->GetVariableByIndex(j);
			D3D11_SHADER_VARIABLE_DESC VarDesc;
			VarReflection->GetDesc(&VarDesc);

			FInternalConstantBufferVariable Variable;
			Variable.Name = VarDesc.Name;
			Variable.Offset = VarDesc.StartOffset;
			Variable.Size = VarDesc.Size;

			// 변수 타입 정보
			ID3D11ShaderReflectionType* TypeReflection = VarReflection->GetType();
			D3D11_SHADER_TYPE_DESC TypeDesc;
			TypeReflection->GetDesc(&TypeDesc);
			Variable.Type = TypeDesc.Class;
			Variable.Elements = TypeDesc.Elements;
			Variable.Columns = TypeDesc.Columns;
			Variable.Rows = TypeDesc.Rows;

			BufferInfo.Variables.push_back(Variable);
		}

		// D3D 상수 버퍼 생성
		D3D11_BUFFER_DESC D3DBufferDesc = {};
		D3DBufferDesc.ByteWidth = BufferDesc.Size;
		D3DBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		D3DBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		D3DBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		ID3D11Buffer* Buffer = nullptr;
		if (SUCCEEDED(Device->CreateBuffer(&D3DBufferDesc, nullptr, &Buffer)))
		{
			BufferInfo.Buffer = Buffer;
			OutBuffers.push_back(BufferInfo);
		}
	}
}

HRESULT UShader::CompileShader(const wchar_t* filename, const char* entryPoint, const char* target, ID3DBlob** ppBlob)
{
	ID3DBlob* pBlob = nullptr;
	ID3DBlob* pErrorBlob = nullptr;

	//UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
	UINT compileFlags = D3DCOMPILE_DEBUG;

	HRESULT hr = D3DCompileFromFile(filename, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
									entryPoint, target, compileFlags, 0, &pBlob, &pErrorBlob);
	if (FAILED(hr))
	{
		if (pErrorBlob)
		{
			const char* errorMsg = (const char*)pErrorBlob->GetBufferPointer();
			LOG("Shader compile error in %S: %s", filename, errorMsg);
			OutputDebugStringA(errorMsg);
			pErrorBlob->Release();
		}
		else
		{
			LOG("Failed to compile shader %S with unknown error", filename);
		}
		*ppBlob = nullptr;
		return hr;
	}

	*ppBlob = pBlob;
	return S_OK;
}

 // 쉐이더 리소스 바인딩 정보 추출
void UShader::ExtractResourceBindings(ID3D11ShaderReflection* Reflection, std::vector<FInternalResourceBindInfo>& OutBindings)
{
	D3D11_SHADER_DESC ShaderDesc;
	Reflection->GetDesc(&ShaderDesc);

	for (UINT i = 0; i < ShaderDesc.BoundResources; i++)
	{
		D3D11_SHADER_INPUT_BIND_DESC BindDesc;
		Reflection->GetResourceBindingDesc(i, &BindDesc);

		// 리소스 타입에 따른 필터링
		switch (BindDesc.Type)
		{
			case D3D_SIT_TEXTURE:       // 텍스처
			case D3D_SIT_UAV_RWTYPED:   // UAV
			case D3D_SIT_SAMPLER:       // 샘플러
			case D3D_SIT_STRUCTURED:    // 구조화 버퍼
			{
				FInternalResourceBindInfo Binding;
				Binding.Name = BindDesc.Name;
				Binding.Type = BindDesc.Type;
				Binding.BindPoint = BindDesc.BindPoint;
				Binding.BindCount = BindDesc.BindCount;
				OutBindings.push_back(Binding);
				break;
			}
			// 상수 버퍼와 기타 타입은 무시
			case D3D_SIT_CBUFFER:
			case D3D_SIT_TBUFFER:
			default:
				break;
		}
	}
}


