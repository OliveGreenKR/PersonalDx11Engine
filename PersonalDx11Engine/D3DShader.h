#pragma once
#include "D3D.h"
#include <vector>

using namespace std;

class FD3DShader
{
public:
	FD3DShader() = default;
	~FD3DShader();

public:
	bool Initialize(ID3D11Device* Device, const wchar_t* vertexShaderPath, const wchar_t* pixelShaderPath, D3D11_INPUT_ELEMENT_DESC* layout, unsigned int layoutSize);
	void Shutdown();

	void Bind(ID3D11DeviceContext* DeviceContext);


	template<typename T>
	void UpdateConstantBuffer(ID3D11DeviceContext* DeviceContext,const T& BufferData, unsigned int BufferIndex = 0);
	void SetTexture(ID3D11DeviceContext* DeviceContext,ID3D11ShaderResourceView* TextureView);
	void SetSamplerState(ID3D11DeviceContext* DeviceContext,ID3D11SamplerState* SamplerState);
	
private:
	ID3D11VertexShader* VertexShader = nullptr;
	ID3D11PixelShader* PixelShader = nullptr;
	ID3D11InputLayout* InputLayout = nullptr;
	vector<ID3D11Buffer*> ConstantBuffers;

};

template<typename T>
inline void FD3DShader::UpdateConstantBuffer(ID3D11DeviceContext* DeviceContext, const T& BufferData, unsigned int BufferIndex)
{
	static_assert(sizeof(T) % 16 == 0, "Constant buffer size must be 16-byte aligned");

	if (BufferIndex >= ConstantBuffers.Num())
	{
		return;
	}

	D3D11_MAPPED_SUBRESOURCE MappedResource;
	HRESULT result = DeviceContext->Map(ConstantVuffers[BufferIndx], 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);

	if (FAILED(result))
		return;
	//from CPU to GPU memory
	memcpy(MappedResource.pData, &BufferData, sizeof(T));
	DeviceContext->Unmap(ConstantBuffers[BufferIndex], 0);
}
