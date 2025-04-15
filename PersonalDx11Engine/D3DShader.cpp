#include "D3DShader.h"
#include "d3d11shader.h"
#include "Debug.h"
#include <map>

UShaderBase::~UShaderBase()
{
	Release();
}

bool UShaderBase::FillShaderMeta(ID3D11Device* Device, ID3DBlob* ShaderBlob)
{
	if (!ShaderBlob || !Device)
	{
		return false;
	}

	// 2. 리플렉션 객체 생성
	ID3D11ShaderReflection* SReflection = nullptr;
	if (FAILED(D3DReflect(ShaderBlob->GetBufferPointer(), ShaderBlob->GetBufferSize(),
						  IID_ID3D11ShaderReflection, (void**)&SReflection)))
	{
		ShaderBlob->Release();
		return false;
	}

	//  입력 레이아웃 자동 생성
	std::vector<D3D11_INPUT_ELEMENT_DESC> InputLayoutDesc;
	if (!CreateInputLayoutFromReflection(SReflection, InputLayoutDesc))
	{
		SReflection->Release();
		ShaderBlob->Release();
		return false;
	}

	if (FAILED(Device->CreateInputLayout(InputLayoutDesc.data(), InputLayoutDesc.size(),
										 ShaderBlob->GetBufferPointer(), ShaderBlob->GetBufferSize(), &InputLayout)))
	{
		SReflection->Release();
		ShaderBlob->Release();
	}

	// 상수 버퍼 정보 추출 및 생성
	ExtractAndCreateConstantBuffers(Device, SReflection, ConstantBuffers);

	// 쉐이더 리소스 정보 추출
	ExtractResourceBindings(SReflection, ResourceBindingMeta);


	SReflection->Release();
	return true;
}

void UShaderBase::Release()
{
	// 상수 버퍼 해제
	for (auto& cbInfo : ConstantBuffers)
	{
		if (cbInfo.Buffer)
		{
			cbInfo.Buffer->Release();
			cbInfo.Buffer = nullptr;
		}
	}
	ConstantBuffers.clear();

	// 입력 레이아웃 해제
	if (InputLayout)
	{
		InputLayout->Release();
		InputLayout = nullptr;
	}

	ResourceBindingMeta.clear();
}

void UShaderBase::CalculateMemoryUsage()
{
	MemorySize = 0;

	// 입력 레이아웃 메모리 (추정치)
	if (InputLayout) MemorySize += 64;

	// 상수 버퍼 메모리
	for (const auto& cbInfo : ConstantBuffers)
	{
		if (cbInfo.Buffer)
		{
			MemorySize += cbInfo.Size;  // 버퍼 자체의 크기
			MemorySize += 64;           // D3D 리소스 오버헤드 (추정치)
		}
	}

	// 메타데이터 메모리 (추정치)
	MemorySize += ResourceBindingMeta.size() * 32;
}

ID3D11Buffer* UShaderBase::GetConstantBuffer(uint32_t Slot) const
{
	if (Slot < ConstantBuffers.size())
		return ConstantBuffers[Slot].Buffer;
	return nullptr;
}

uint32_t UShaderBase::GetConstantBufferSize(uint32_t Slot) const
{
	if (Slot < ConstantBuffers.size())
		return ConstantBuffers[Slot].Size;
	return 0;
}

const std::string& UShaderBase::GetConstantBufferName(uint32_t Slot) const
{
	static const std::string EmptyString;
	if (Slot < ConstantBuffers.size())
		return ConstantBuffers[Slot].Name;
	return EmptyString;
}


bool UShaderBase::CreateInputLayoutFromReflection(ID3D11ShaderReflection* Reflection, std::vector<D3D11_INPUT_ELEMENT_DESC>& OutLayout)
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
void UShaderBase::ExtractAndCreateConstantBuffers(ID3D11Device* Device, ID3D11ShaderReflection* Reflection, std::vector<FInternalConstantBufferInfo>& OutBuffers)
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

HRESULT UShaderBase::CompileShader(const wchar_t* filename, const char* entryPoint, const char* target, ID3DBlob** ppBlob)
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
void UShaderBase::ExtractResourceBindings(ID3D11ShaderReflection* Reflection, std::vector<FInternalResourceBindInfo>& OutBindings)
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


