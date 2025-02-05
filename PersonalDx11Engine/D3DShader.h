#pragma once

#include "D3D.h"
#include <vector>
#include <type_traits>

using namespace std;

struct alignas(16) FMatrixBufferData
{
	XMMATRIX World;      // 월드 변환 행렬
	XMMATRIX View;       // 뷰 변환 행렬 
	XMMATRIX Projection; // 투영 변환 행렬

	FMatrixBufferData()
		: World(XMMatrixIdentity())
		, View(XMMatrixIdentity())
		, Projection(XMMatrixIdentity())
	{}
	FMatrixBufferData(const XMMATRIX& world, const XMMATRIX& view, const XMMATRIX& projection) :
		World(world), View(view), Projection(projection) {};
};

struct alignas(16) FDebugBufferData
{
	XMFLOAT4 DebugColor = XMFLOAT4(1, 1, 1, 1);

	FDebugBufferData() = default;
	FDebugBufferData(const XMFLOAT4 InColor) : DebugColor(InColor) {}
	FDebugBufferData(float r, float g, float b, float a) 
	{
		DebugColor = XMFLOAT4(r, g, b, a);
	};
};

enum class ETextureSlot
{
	Albedo = 0,
	Normal,
	Specular,
	Max,
};

enum class EBufferSlot
{
	Matrix = 0,
	DebugColor,
	Max
};
/// <summary>
/// 단일 SamplerState, 복수의 cBuffer를 지원하는 쉐이더 파일 관리
/// </summary>
class UShader
{
private:


public:
	UShader() = default;
	~UShader();

public:
	void Initialize(ID3D11Device* Device, const wchar_t* vertexShaderPath, const wchar_t* pixelShaderPath, D3D11_INPUT_ELEMENT_DESC* layout, const unsigned int layoutSize);
	void Release();
	//파이프라인 상태 설정, vs, ps, samplerstate
	void Bind(ID3D11DeviceContext* DeviceContext, ID3D11SamplerState* InSamplerState = nullptr);
	void BindTexture(ID3D11DeviceContext* DeviceContext, ID3D11ShaderResourceView* Texture , ETextureSlot Slot);
	void BindMatrix(ID3D11DeviceContext* DeviceContext, FMatrixBufferData& Data);
	void BindColor(ID3D11DeviceContext* DeviceContext, FDebugBufferData& Data);
	__forceinline const bool IsInitialized() const { return bIsInitialized; }

public:
	template<typename T>
	void UpdateConstantBuffer(ID3D11DeviceContext* DeviceContext, T& BufferData, const EBufferSlot BufferIndex = 0);

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
inline void UShader::UpdateConstantBuffer(ID3D11DeviceContext* DeviceContext, T& BufferData, const EBufferSlot BufferIndex)
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
