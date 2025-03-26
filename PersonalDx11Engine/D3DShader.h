#pragma once

#include "RenderHardwareInterface.h"
#include "ResourceInterface.h"
#include "ShaderInterface.h"
#include "Math.h"
#include <vector>
#include <string>
#include <type_traits>

using namespace std;


class UShader : public IResource, public IShader
{
private:
	// 상수 버퍼 변수 정보
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
	// 상수 버퍼  정보
	struct FConstantBufferInfo
	{
		std::string Name;
		UINT Size;
		UINT BindPoint;
		ID3D11Buffer* Buffer = nullptr;
		std::vector<FConstantBufferVariable> Variables;
	};
	// 리소스 바인딩 정보
	struct FResourceBindInfo
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

	std::vector<FResourceBindInfo> VSResourceBindings;
	std::vector<FResourceBindInfo> PSResourceBindings;

public:
	UShader() = default;
	~UShader();

	bool Load(ID3D11Device* Device, const wchar_t* VSPath, const wchar_t* PSPath);

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

public:
	//Inherited by IShader
	ID3D11VertexShader* GetVertexShader() const override { return VertexShader; }
	ID3D11PixelShader* GetPixelShader() const override { return PixelShader; }
	ID3D11InputLayout* GetInputLayout() const override { return InputLayout; }

	std::vector<ConstantBufferInfo> GetVSConstantBufferInfos() const override
	{
		std::vector<ConstantBufferInfo> result;
		for (const auto& cb : VSConstantBuffers)
		{
			result.push_back({ cb.BindPoint, cb.Buffer, cb.Size, cb.Name });
		}
			
		return result;
	}

	std::vector<ConstantBufferInfo> GetPSConstantBufferInfos() const override
	{
		std::vector<ConstantBufferInfo> result;
		for (const auto& cb : PSConstantBuffers)
			result.push_back({ cb.BindPoint, cb.Buffer, cb.Size, cb.Name });
		return result;
	}

	//SRV는 외부에서 설정해야함
	std::vector<TextureBindingInfo> GetTextureInfos() const override
	{
		std::vector<TextureBindingInfo> result;
		for (const auto& binding : VSResourceBindings)
			if (binding.Type == D3D_SIT_TEXTURE)
				result.push_back({ binding.BindPoint, binding.Name }); // SRV는 외부에서 설정
		for (const auto& binding : PSResourceBindings)
			if (binding.Type == D3D_SIT_TEXTURE)
				result.push_back({ binding.BindPoint, binding.Name });
		return result;
	}

	//Sampler는 외부에서 설정해야함
	std::vector<SamplerBindingInfo> GetSamplerInfos() const override
	{
		std::vector<SamplerBindingInfo> result;
		for (const auto& binding : VSResourceBindings)
			if (binding.Type == D3D_SIT_SAMPLER)
				result.push_back({ binding.BindPoint, binding.Name }); // Sampler는 외부에서 설정
		for (const auto& binding : PSResourceBindings)
			if (binding.Type == D3D_SIT_SAMPLER)
				result.push_back({ binding.BindPoint, binding.Name });
		return result;
	}

	// 상수 버퍼 레이아웃 정보
	const std::vector<FConstantBufferInfo>& GetVSConstantBufferInfo() const { return VSConstantBuffers; }
	const std::vector<FConstantBufferInfo>& GetPSConstantBufferInfo() const { return PSConstantBuffers; }

	/// <param name="Slot"> '-1'일 경우 이름을 통해 검색, 슬롯이 명시될 경우 슬롯으로 검색됨</param>
	/// <returns></returns>
	bool BindTexture(ID3D11DeviceContext* Context, ID3D11ShaderResourceView* SRV,
					 uint32_t Slot, const std::string& ResourceName = "");

	/// </summary>
	/// <param name="Slot"> '-1'일 경우 이름을 통해 검색, 슬롯이 명시될 경우 슬롯으로 검색됨</param>
	/// <returns></returns>
	bool BindSampler(ID3D11DeviceContext* Context, ID3D11SamplerState* Sampler,
					 uint32_t Slot, const std::string& SamplerName = "");

	//전체 상수 버퍼 업데이트
	bool UpdateConstantBuffer(ID3D11DeviceContext* Context, const std::string& BufferName, const void* Data, uint32_t DataSize);

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
								 std::vector<FResourceBindInfo>& OutBindings);
};