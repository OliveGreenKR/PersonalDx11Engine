#pragma once

#include "D3D.h"
#include <vector>

enum class TextureSlot
{
	Diffuset = 0,
	Normal,
	Specular,
	Max,
};

using namespace std;
/// <summary>
/// 단일 SamplerState, 복수의 cBuffer를 지원하는 쉐이더 파일 관리
/// </summary>
class UShader
{
public:
	UShader() = default;
	~UShader();

public:
	void Initialize(ID3D11Device* Device, const wchar_t* vertexShaderPath, const wchar_t* pixelShaderPath, D3D11_INPUT_ELEMENT_DESC* layout, const unsigned int layoutSize);
	void Release();
	//파이프라인 상태 설정, vs, ps, samplerstate
	void Bind(ID3D11DeviceContext* DeviceContext, ID3D11SamplerState* InSamplerState = nullptr);
	void BindTexture(ID3D11DeviceContext* DeviceContext, ID3D11ShaderResourceView* Texture = nullptr, TextureSlot Slot = TextureSlot::Max);

	template<typename T>
	void BindConstantBuffer(ID3D11DeviceContext* DeviceContext,const T& BufferData, UINT BufferIndex = 0);

	__forceinline const bool IsInitialized() const { return bIsInitialized; }

private:
	void SetSamplerState(ID3D11SamplerState* InSamplerState);
private:
	ID3D11VertexShader* VertexShader = nullptr;
	ID3D11PixelShader* PixelShader = nullptr;
	ID3D11InputLayout* InputLayout = nullptr;

	vector<ID3D11Buffer*> ConstantBuffers;
	ID3D11SamplerState* SamplerState = nullptr;

	bool bIsInitialized = false;
};

template<typename T>
inline void UShader::BindConstantBuffer(ID3D11DeviceContext* DeviceContext, const T& BufferData, UINT BufferIndex)
{
	static_assert(sizeof(T) % 16 == 0, "Constant buffer size must be 16-byte aligned");

	if (!DeviceContext || BufferIndex >= ConstantBuffers.size())
	{
		return;
	}

	D3D11_MAPPED_SUBRESOURCE MappedResource;
	HRESULT result = DeviceContext->Map(ConstantBuffers[BufferIndex], 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);

	if (FAILED(result))
		return;

	//from CPU to GPU memory
	memcpy(MappedResource.pData, &BufferData, sizeof(T));
	DeviceContext->Unmap(ConstantBuffers[BufferIndex], 0);

	//constant buffer bind
	DeviceContext->VSSetConstantBuffers(BufferIndex, 1, &ConstantBuffers[BufferIndex]);
}
