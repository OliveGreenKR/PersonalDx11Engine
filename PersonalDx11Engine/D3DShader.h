#pragma once

#include "RenderHardwareInterface.h"
#include "ResourceInterface.h"
#include "Math.h"
#include <vector>
#include <string>
#include <type_traits>

using namespace std;


class UShader : public IResource
{
private:

	// 상수 버퍼 정보
	struct FConstantBufferVariable
	{
		std::string Name;
		UINT Offset;
		UINT Size;
		D3D_SHADER_VARIABLE_CLASS Type;
		UINT Elements;
		UINT Rows;
		UINT Columns;
	};

	struct FConstantBufferInfo
	{
		std::string Name;
		UINT Size;
		UINT BindPoint;
		ID3D11Buffer* Buffer = nullptr;
		std::vector<FConstantBufferVariable> Variables;
	};

	// 리소스 바인딩 정보
	struct FResourceBinding
	{
		std::string Name;
		D3D_SHADER_INPUT_TYPE Type;
		UINT BindPoint;
		UINT BindCount;
	};

private:
	ID3D11VertexShader* VertexShader;
	ID3D11PixelShader* PixelShader;
	ID3D11InputLayout* InputLayout;
	ID3DBlob* VSByteCode;

	std::vector<FConstantBufferInfo> VSConstantBuffers;
	std::vector<FConstantBufferInfo> PSConstantBuffers;

	bool bIsLoaded = false;
	size_t MemorySize = 0;

	std::vector<FResourceBinding> VSResourceBindings;
	std::vector<FResourceBinding> PSResourceBindings;

public:
	bool Load(ID3D11Device* Device, const wchar_t* VSPath, const wchar_t* PSPath);

	// 기본 접근자 메서드
	ID3D11VertexShader* GetVertexShader() const { return VertexShader; }
	ID3D11PixelShader* GetPixelShader() const	{ return PixelShader; }
	ID3D11InputLayout* GetInputLayout() const	{ return InputLayout; }


	ID3D11Buffer* GetVSConstantBuffer(uint32_t Slot) const
	{
		if (Slot < VSConstantBuffers.size())
		{
			return VSConstantBuffers[Slot].Buffer;
		}
		return nullptr;
	}
	uint32_t GetVSConstantBufferSize(uint32_t Slot) const
	{
		if (Slot < VSConstantBuffers.size())
		{
			return VSConstantBuffers[Slot].Size;
		}
		return 0;
	}
	const std::string& GetVSConstantBufferName(uint32_t Slot) const
	{
		if (Slot < VSConstantBuffers.size())
		{
			return VSConstantBuffers[Slot].Name;
		}
		return std::string();
	}

	ID3D11Buffer* GetPSConstantBuffer(uint32_t Slot) const
	{
		if (Slot < PSConstantBuffers.size())
		{
			return PSConstantBuffers[Slot].Buffer;
		}
		return nullptr;
	}
	uint32_t GetPSConstantBufferSize(uint32_t Slot) const
	{
		if (Slot < PSConstantBuffers.size())
		{
			return PSConstantBuffers[Slot].Size;
		}
		return 0;
	}
	const std::string& GetPSConstantBufferName(uint32_t Slot) const
	{
		if (Slot < PSConstantBuffers.size())
		{
			return PSConstantBuffers[Slot].Name;
		}
		return std::string();
	}

	// 상수 버퍼 레이아웃 정보
	const std::vector<FConstantBufferInfo>& GetVSConstantBufferInfo() const { return VSConstantBuffers; }
	const std::vector<FConstantBufferInfo>& GetPSConstantBufferInfo() const { return PSConstantBuffers; }

	// 이름으로 상수 버퍼 변수 업데이트
	template<typename T>
	bool UpdateConstantBufferVariable(ID3D11DeviceContext* Context, const std::string& BufferName,
									  const std::string& VariableName, const T& Value);

public:
	UShader() = default;
	~UShader();

public:
	// IResource 인터페이스 구현
	bool IsLoaded() const override { return bIsLoaded; }
	void Release() override;
	size_t GetMemorySize() const override { return MemorySize; }

private:
	bool CreateInputLayoutFromReflection(ID3D11ShaderReflection* Reflection,
										 std::vector<D3D11_INPUT_ELEMENT_DESC>& OutLayout);
	void ExtractAndCreateConstantBuffers(ID3D11Device* Device,
										 ID3D11ShaderReflection* Reflection,
										 std::vector<FConstantBufferInfo>& OutBuffers);

	HRESULT CompileShader(const wchar_t* filename, const char* entryPoint, const char* target, ID3DBlob** ppBlob);

	void ExtractResourceBindings(ID3D11ShaderReflection* Reflection,
								 std::vector<FResourceBinding>& OutBindings);
};

template<typename T>
inline bool UShader::UpdateConstantBufferVariable
(ID3D11DeviceContext* Context, const std::string& BufferName, const std::string& VariableName, const T& Value)
{
	// 버퍼 찾기
	FConstantBufferInfo* BufferInfo = nullptr;
	for (auto& Buffer : VSConstantBuffers)
	{
		if (Buffer.Name == BufferName)
		{
			BufferInfo = &Buffer;
			break;
		}
	}

	if (!BufferInfo)
	{
		for (auto& Buffer : PSConstantBuffers)
		{
			if (Buffer.Name == BufferName)
			{
				BufferInfo = &Buffer;
				break;
			}
		}
	}

	if (!BufferInfo || !BufferInfo->Buffer)
		return false;

	// 변수 찾기
	FConstantBufferVariable* Variable = nullptr;
	for (auto& Var : BufferInfo->Variables)
	{
		if (Var.Name == VariableName)
		{
			Variable = &Var;
			break;
		}
	}

	if (!Variable || Variable->Size < sizeof(T))
		return false;

	// 상수 버퍼 매핑
	D3D11_MAPPED_SUBRESOURCE MappedResource;
	if (FAILED(Context->Map(BufferInfo->Buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource)))
		return false;

	// 변수 데이터 업데이트
	memcpy((uint8_t*)MappedResource.pData + Variable->Offset, &Value, sizeof(T));

	// 언매핑
	Context->Unmap(BufferInfo->Buffer, 0);

	// 쉐이더에 바인딩
	if (std::find(VSConstantBuffers.begin(), VSConstantBuffers.end(), *BufferInfo) != VSConstantBuffers.end())
		Context->VSSetConstantBuffers(BufferInfo->BindPoint, 1, &BufferInfo->Buffer);
	else
		Context->PSSetConstantBuffers(BufferInfo->BindPoint, 1, &BufferInfo->Buffer);

	return true;
}