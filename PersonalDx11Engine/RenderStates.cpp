#include "RenderStates.h"
#include "Debug.h"


FSolidState::~FSolidState()
{
	if (PreviousSamplerState)
	{
		PreviousSamplerState->Release();
		PreviousSamplerState = nullptr;
	}
	if (SolidSamplerState)
	{
		SolidSamplerState->Release();
		SolidSamplerState = nullptr;
	}
	if (SolidRasterizerState)
	{
		SolidRasterizerState->Release();
		SolidRasterizerState = nullptr;
	}
	if (PreviousRasterizerState)
	{
		PreviousRasterizerState->Release();
		PreviousRasterizerState = nullptr;
	}
}

std::unique_ptr<FSolidState> FSolidState::Create(ID3D11Device* Device)
{
	if (!Device)
		return nullptr;

	auto SolidState = std::make_unique<FSolidState>();

	//rasterizer
	D3D11_RASTERIZER_DESC solidDesc = {};
	solidDesc.FillMode = D3D11_FILL_SOLID;
	solidDesc.CullMode = D3D11_CULL_BACK;

	HRESULT hr = Device->CreateRasterizerState(&solidDesc, &SolidState->SolidRasterizerState);

	if (FAILED(hr))
	{
		SolidState = nullptr;
		return nullptr;
	}

	//sampler
	D3D11_SAMPLER_DESC samplerDesc = {};
	//samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;  // 바이리니어 필터링, 부드러운 텍스처 표시
	samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;          // 비등방성 필터링 사용
	samplerDesc.MaxAnisotropy = 16;                       //비등방성 필터링 수준
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;  // 비교 샘플링 사용 안함
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;     // U좌표 반복
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;     // V좌표 반복  
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;     // W좌표 반복
	samplerDesc.MinLOD = 0;                                // 최소 LOD 레벨
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;               // 최대 LOD 제한 없음
	samplerDesc.MipLODBias = 0;                           // LOD 레벨 조정 없음

	hr = Device->CreateSamplerState(&samplerDesc, &SolidState->SolidSamplerState);

	if (FAILED(hr))
	{
		LOG_FUNC_CALL("Failed CreateSamelerStates for SolidStates");
		return nullptr;
	}

	return SolidState;

}

FWireframeState::~FWireframeState()
{
	if (WireframeRasterizerState)
	{
		WireframeRasterizerState->Release();
	}
	if (PreviousRasterizerState)
	{
		PreviousRasterizerState->Release();
	}
}

std::unique_ptr<FWireframeState> FWireframeState::Create(ID3D11Device* Device)
{
	if (!Device)
		return nullptr;

	auto WireState = std::make_unique<FWireframeState>();

	// 와이어프레임 래스터라이저 상태
	D3D11_RASTERIZER_DESC wireframeDesc = {};
	wireframeDesc.FillMode = D3D11_FILL_WIREFRAME;
	wireframeDesc.CullMode = D3D11_CULL_NONE;
	wireframeDesc.AntialiasedLineEnable = FALSE;
	wireframeDesc.MultisampleEnable = FALSE;
	wireframeDesc.DepthClipEnable = TRUE;

	HRESULT hr = Device->CreateRasterizerState(&wireframeDesc, &WireState->WireframeRasterizerState);

	if (FAILED(hr))
	{
		return nullptr;
	}

	return WireState;
}
