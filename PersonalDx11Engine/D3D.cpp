#include "D3D.h"

FD3D::~FD3D()
{
	Release();
}

bool FD3D::Initialize(HWND Hwnd)
{
	return CreateDeviceAndSwapChain(Hwnd) && 
		CreateFrameBuffer() &&
		CreateRasterizerStateAndMatricies();
}

void FD3D::Release()
{
	if (RasterizerState)
	{
		RasterizerState->Release();
	}
	if (DeviceContext)
	{
		DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
	}
	ReleaseFrameBuffer();
	ReleaseDeviceAndSwapChain();
}

void FD3D::BeginScene()
{
	PrepareRender();
}

void FD3D::EndScene()
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

void FD3D::PrepareRender()
{
	DeviceContext->ClearRenderTargetView(FrameBufferRTV, ClearColor);

	DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	DeviceContext->RSSetViewports(1, &ViewportInfo);
	DeviceContext->RSSetState(RasterizerState);

	DeviceContext->OMSetRenderTargets(1, &FrameBufferRTV, nullptr);
	DeviceContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);
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

bool FD3D::CreateRasterizerStateAndMatricies()
{
	D3D11_RASTERIZER_DESC rasterizerdesc = {};
	rasterizerdesc.FillMode = D3D11_FILL_SOLID; // 채우기 모드
	rasterizerdesc.CullMode = D3D11_CULL_BACK; // 백 페이스 컬링

	return !FAILED(Device->CreateRasterizerState(&rasterizerdesc, &RasterizerState));
}

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
