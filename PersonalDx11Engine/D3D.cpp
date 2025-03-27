#include "D3D.h"

FD3D::~FD3D()
{
	Release();
}

bool FD3D::Initialize(HWND Hwnd)
{
	if (bIsInitialized)
		return true;

	bool result = CreateDeviceAndSwapChain(Hwnd) &&
		CreateFrameBuffer() &&
		CreateRasterizerState() &&
		CreateDpethStencilBuffer() &&
		CreateDepthStencilState() &&
		CreateDepthStencillView() &&
		CreateBlendState();
	bIsInitialized = result;
	assert(result);
	InitRenderContext();
	return result;
}

void FD3D::Release()
{
	ReleaseRasterizerState();
	ReleaseFrameBuffer();
	ReleaseDeviceAndSwapChain();
	ReleaseDepthStencil();
	if (DeviceContext)
	{
		DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
	} 
}

void FD3D::BeginFrame()
{
	DeviceContext->ClearRenderTargetView(FrameBufferRTV, ClearColor);
	DeviceContext->ClearDepthStencilView(DepthStencilView,
									 D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	//for ImGui
	DeviceContext->OMSetRenderTargets(1, &FrameBufferRTV, DepthStencilView);
}

void FD3D::EndFrame()
{
	//swap buffer
	SwapChain->Present(bVSync, 0);
}

bool FD3D::CopyBuffer(ID3D11Buffer* SrcBuffer, OUT ID3D11Buffer** DestBuffer)
{
	if (!Device || !SrcBuffer || !DestBuffer || !DeviceContext)
		return false;

	D3D11_BUFFER_DESC bufferDesc;
	SrcBuffer->GetDesc(&bufferDesc);

	D3D11_SUBRESOURCE_DATA initData;
	initData.pSysMem = nullptr;

	// Create a staging buffer to copy the data
	ID3D11Buffer* stagingBuffer = nullptr;
	bufferDesc.Usage = D3D11_USAGE_STAGING;
	bufferDesc.BindFlags = 0;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	HRESULT hr = Device->CreateBuffer(&bufferDesc, nullptr, &stagingBuffer);
	if (FAILED(hr))
		return false;

	DeviceContext->CopyResource(stagingBuffer, SrcBuffer);

	// Map the staging buffer to read the data
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	hr = DeviceContext->Map(stagingBuffer, 0, D3D11_MAP_READ, 0, &mappedResource);
	if (FAILED(hr))
	{
		stagingBuffer->Release();
		return false;
	}

	// Create the destination buffer with the same description as the source buffer
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.CPUAccessFlags = 0;
	initData.pSysMem = mappedResource.pData;
	hr = Device->CreateBuffer(&bufferDesc, &initData, DestBuffer);

	// Unmap the staging buffer and release it
	DeviceContext->Unmap(stagingBuffer, 0);
	stagingBuffer->Release();

	return SUCCEEDED(hr);
}

void FD3D::InitRenderContext()
{
	//Input Assembly
	DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	//Rasterizer
	DeviceContext->RSSetViewports(1, &ViewportInfo);
	DeviceContext->RSSetState(RasterizerState);
	//OutputMerge
	DeviceContext->OMSetBlendState(BlendState, nullptr, 0xffffffff);
	DeviceContext->OMSetDepthStencilState(DepthStencilState, 1);
}

bool FD3D::CreateDeviceAndSwapChain(HWND Hwnd)
{
	HRESULT result;

	D3D_FEATURE_LEVEL featurelevels[] = { D3D_FEATURE_LEVEL_11_0 };

	DXGI_SWAP_CHAIN_DESC swapchaindesc = {};
	swapchaindesc.BufferDesc.Width = 0; // 창 크기에 맞게 자동으로 설정
	swapchaindesc.BufferDesc.Height = 0; // 창 크기에 맞게 자동으로 설정
	swapchaindesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // 색상 포맷 - 32bit
	swapchaindesc.SampleDesc.Count = 1; // 멀티 샘플링 비활성화
	swapchaindesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // 렌더 타겟으로 사용
	swapchaindesc.BufferCount = 2; // 더블 버퍼링
	swapchaindesc.OutputWindow = Hwnd; // 렌더링할 창 핸들
	swapchaindesc.Windowed = TRUE; // 창 모드
	swapchaindesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // 스왑 방식 - discard backbuffer
	swapchaindesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED; //auto selected by G-Driver
	swapchaindesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED; ////auto selected by G-Driver

	result = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
										   D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG,
										   featurelevels, ARRAYSIZE(featurelevels), D3D11_SDK_VERSION,
										   &swapchaindesc, &SwapChain, &Device, nullptr, &DeviceContext);

	if (FAILED(result))
	{
		//CPU rengdering
		result = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr,
											   D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG,
											   featurelevels, ARRAYSIZE(featurelevels), D3D11_SDK_VERSION,
											   &swapchaindesc, &SwapChain, &Device, nullptr, &DeviceContext);
		if (FAILED(result))
		{
			return false;
		}
	}

	SwapChain->GetDesc(&swapchaindesc);

	ViewportInfo.MinDepth = 0.0f;
	ViewportInfo.MaxDepth = 1.0f;
	ViewportInfo.TopLeftX = 0.0f;
	ViewportInfo.TopLeftY = 0.0f;
	ViewportInfo.Width = (float)swapchaindesc.BufferDesc.Width;
	ViewportInfo.Height = (float)swapchaindesc.BufferDesc.Height;

	return true;
}

bool FD3D::CreateFrameBuffer()
{
	SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&FrameBuffer);

	D3D11_RENDER_TARGET_VIEW_DESC framebufferRTVdesc = {};
	framebufferRTVdesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB; //32bit
	framebufferRTVdesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D; // 2D Texture

	HRESULT result = Device->CreateRenderTargetView(FrameBuffer, &framebufferRTVdesc, &FrameBufferRTV);

	return SUCCEEDED(result);
}

bool FD3D::CreateRasterizerState()
{
	D3D11_RASTERIZER_DESC rasterizerdesc = {};
	rasterizerdesc.FillMode = D3D11_FILL_SOLID; // 채우기 모드
	rasterizerdesc.CullMode = D3D11_CULL_BACK; // 백 페이스 컬링

	return SUCCEEDED(Device->CreateRasterizerState(&rasterizerdesc, &RasterizerState));
}

bool FD3D::CreateDpethStencilBuffer()
{
	D3D11_TEXTURE2D_DESC depthBufferDesc = {};
	depthBufferDesc.Width = static_cast<UINT>(ViewportInfo.Width);
	depthBufferDesc.Height = static_cast<UINT>(ViewportInfo.Height);
	depthBufferDesc.MipLevels = 1;
	depthBufferDesc.ArraySize = 1;
	depthBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthBufferDesc.SampleDesc.Count = 1;
	depthBufferDesc.SampleDesc.Quality = 0;
	depthBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthBufferDesc.CPUAccessFlags = 0;
	depthBufferDesc.MiscFlags = 0;

	HRESULT hr = Device->CreateTexture2D(&depthBufferDesc, nullptr, &DepthStencilBuffer);
	return SUCCEEDED(hr);
}

bool FD3D::CreateDepthStencilState()
{
	D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
	depthStencilDesc.DepthEnable = TRUE;
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
	//depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

	// Stencil test parameters
	depthStencilDesc.StencilEnable = TRUE;
	depthStencilDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	depthStencilDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;

	// Stencil operations if pixel is front-facing
	depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Stencil operations if pixel is back-facing
	depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	HRESULT hr = Device->CreateDepthStencilState(&depthStencilDesc, &DepthStencilState);
	return SUCCEEDED(hr);
}

bool FD3D::CreateDepthStencillView()
{
	D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {};
	depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	depthStencilViewDesc.Texture2D.MipSlice = 0;

	HRESULT hr = Device->CreateDepthStencilView(DepthStencilBuffer, &depthStencilViewDesc, &DepthStencilView);
	return SUCCEEDED(hr);
}

bool FD3D::CreateBlendState()
{
	D3D11_BLEND_DESC blendDesc = {};
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	return SUCCEEDED(Device->CreateBlendState(&blendDesc, &BlendState));
}

//bool FD3D::CreateDefaultSamplerState()
//{
//	D3D11_SAMPLER_DESC samplerDesc = {};
//	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;  // 바이리니어 필터링, 부드러운 텍스처 표시
//	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;     // U좌표 반복
//	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;     // V좌표 반복  
//	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;     // W좌표 반복
//	samplerDesc.MinLOD = 0;                                // 최소 LOD 레벨
//	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;               // 최대 LOD 제한 없음
//	samplerDesc.MipLODBias = 0;                           // LOD 레벨 조정 없음
//	samplerDesc.MaxAnisotropy = 1;                        // 비등방성 필터링 사용 안함
//	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;  // 비교 샘플링 사용 안함
//
//	HRESULT result = GetDevice()->CreateSamplerState(&samplerDesc, &DefaultSamplerState);
//	return SUCCEEDED(result);
//}

void FD3D::ReleaseDeviceAndSwapChain()
{
	if (DeviceContext)
	{
		DeviceContext->Flush(); // 남아있는 GPU 명령 실행
	}

	if (SwapChain)
	{
		SwapChain->Release();
		SwapChain = nullptr;
	}

	if (Device)
	{
		Device->Release();
		Device = nullptr;
	}

	if (DeviceContext)
	{
		DeviceContext->Release();
		DeviceContext = nullptr;
	}
}

void FD3D::ReleaseFrameBuffer()
{
	if (FrameBuffer)
	{
		FrameBuffer->Release();
		FrameBuffer = nullptr;
	}

	if (FrameBufferRTV)
	{
		FrameBufferRTV->Release();
		FrameBufferRTV = nullptr;
	}
}

void FD3D::ReleaseRasterizerState()
{
	if (RasterizerState)
	{
		RasterizerState->Release();
		RasterizerState = nullptr;
	}
}

void FD3D::ReleaseDepthStencil()
{
	if (DepthStencilView)
	{
		DepthStencilView->Release();
		DepthStencilView = nullptr;
	}
	if (DepthStencilState)
	{
		DepthStencilState->Release();
		DepthStencilState = nullptr;
	}
	if (DepthStencilBuffer)
	{
		DepthStencilBuffer->Release();
		DepthStencilBuffer = nullptr;
	}
	
}
