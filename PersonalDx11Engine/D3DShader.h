#pragma once

#include "D3D.h"
#include <vector>
#include <type_traits>

enum class ETextureSlot
{
	Diffuset = 0,
	Normal,
	Specular,
	Max,
};

enum class EBufferSlot
{
	ModelMatrix = 0,
	Max
};

struct alignas(16) FMatrixBuffer
{
	XMMATRIX World;      // 월드 변환 행렬
	XMMATRIX View;       // 뷰 변환 행렬 
	XMMATRIX Projection; // 투영 변환 행렬

	FMatrixBuffer()
		: World(XMMatrixIdentity())
		, View(XMMatrixIdentity())
		, Projection(XMMatrixIdentity())
	{
	}
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
	void BindTexture(ID3D11DeviceContext* DeviceContext, ID3D11ShaderResourceView* Texture = nullptr, ETextureSlot Slot = ETextureSlot::Max);

	template<typename T>
	void UpdateConstantBuffer(ID3D11DeviceContext* DeviceContext,const T& BufferData, const EBufferSlot BufferIndex = 0);

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
inline void UShader::UpdateConstantBuffer(ID3D11DeviceContext* DeviceContext, const T& BufferData, const EBufferSlot BufferIndex)
{
	static_assert(sizeof(T) % 16 == 0, "Constant buffer size must be 16-byte aligned");

	UINT idx = static_cast<UINT>(BufferIndex);
	if (!DeviceContext || idx >= ConstantBuffers.size())
	{
		return;
	}

	D3D11_MAPPED_SUBRESOURCE MappedResource;
	HRESULT result = DeviceContext->Map(ConstantBuffers[idx], 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);

	if (FAILED(result))
		return;

	//from CPU to GPU memory
	memcpy(MappedResource.pData, &BufferData, sizeof(T));
	DeviceContext->Unmap(ConstantBuffers[idx], 0);

	//constant buffer bind
	DeviceContext->VSSetConstantBuffers(idx, 1, &ConstantBuffers[idx]);
}
