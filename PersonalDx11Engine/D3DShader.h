#pragma once

#include "RenderHardwareInterface.h"
#include "ResourceInterface.h"
#include "Math.h"
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

struct alignas(16) FColorBufferData
{
	XMFLOAT4 Color = XMFLOAT4(1, 1, 1, 1);

	FColorBufferData() = default;
	FColorBufferData(const XMFLOAT4 InColor) : Color(InColor) {}
	FColorBufferData(float r, float g, float b, float a) 
	{
		Color = XMFLOAT4(r, g, b, a);
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
	Color,
	Max
};
/// <summary>
/// TODO 컴파일된 쉐이더를 리플렉션을 통해 상수 버퍼 업데이트를 하고
/// 디바이스 컨텍스트를 이용해 바인딩하는 것이 주 역할로 변경할것
/// </summary>
class UShader : public IResource
{
private:


public:
	UShader() = default;
	~UShader();

public:

	// IResource 인터페이스 구현
	bool IsLoaded() const override { return bIsLoaded; }
	void Release() override;
	size_t GetMemorySize() const override { return MemorySize; }

	void Load(ID3D11Device* Device, const wchar_t* vertexShaderPath, const wchar_t* pixelShaderPath, D3D11_INPUT_ELEMENT_DESC* layout, const unsigned int layoutSize);
	
	//파이프라인 상태 설정, vs, ps, samplerstate 
	void Bind(ID3D11DeviceContext* DeviceContext, ID3D11SamplerState* InSamplerState = nullptr);
	void BindTexture(ID3D11DeviceContext* DeviceContext, ID3D11ShaderResourceView* Texture , ETextureSlot Slot);
	void BindMatrix(ID3D11DeviceContext* DeviceContext, FMatrixBufferData& Data);
	void BindColor(ID3D11DeviceContext* DeviceContext, FColorBufferData& Data);

	ID3D11InputLayout* GetInputLayout() const { return InputLayout;}

	//쉐이더 바이트 코드 접근자
	void GetShaderBytecode(const void** bytecode, size_t* length) const;

	ID3D11VertexShader* GetVertexShader() { return VertexShader; }
	ID3D11PixelShader* GetPixelShader() { return PixelShader; }
public:
	template<typename T>
	void UpdateConstantBuffer(ID3D11DeviceContext* DeviceContext, T& BufferData, const EBufferSlot BufferIndex = 0);

	void SetSamplerState(ID3D11SamplerState* InSamplerState);

private:

	HRESULT CompileShader(const wchar_t* filename, const char* entryPoint, const char* target, ID3DBlob** ppBlob);

private:
	ID3D11VertexShader* VertexShader = nullptr;
	ID3D11PixelShader* PixelShader = nullptr;
	ID3D11InputLayout* InputLayout = nullptr;

	vector<ID3D11Buffer*> ConstantBuffers;
	ID3D11SamplerState* SamplerState = nullptr;

	ID3DBlob* VSByteCode = nullptr; // 컴파일된 버텍스 셰이더 바이트코드 저장

	bool bIsLoaded = false;
	size_t MemorySize = 0;
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
