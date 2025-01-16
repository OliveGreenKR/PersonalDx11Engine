#include "D3D.h"

FD3D::~FD3D()
{
	Shutdown();
}

bool FD3D::Initialize(HWND Hwnd)
{
	return CreateDeviceAndSwapChain(Hwnd) && 
		CreateFrameBuffer() &&
		CreateRasterizerStateAndMatricies();
}

void FD3D::Shutdown()
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
	swapchaindesc.BufferDesc.Width = 0; // â ũ�⿡ �°� �ڵ����� ����
	swapchaindesc.BufferDesc.Height = 0; // â ũ�⿡ �°� �ڵ����� ����
	swapchaindesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // ���� ���� - 32bit
	swapchaindesc.SampleDesc.Count = 1; // ��Ƽ ���ø� ��Ȱ��ȭ
	swapchaindesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // ���� Ÿ������ ���
	swapchaindesc.BufferCount = 2; // ���� ���۸�
	swapchaindesc.OutputWindow = Hwnd; // �������� â �ڵ�
	swapchaindesc.Windowed = TRUE; // â ���
	swapchaindesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // ���� ��� - discard backbuffer
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

	return !FAILED(Device->CreateRenderTargetView(FrameBuffer, &framebufferRTVdesc, &FrameBufferRTV));
}

bool FD3D::CreateRasterizerStateAndMatricies()
{
	D3D11_RASTERIZER_DESC rasterizerdesc = {};
	rasterizerdesc.FillMode = D3D11_FILL_SOLID; // ä��� ���
	rasterizerdesc.CullMode = D3D11_CULL_BACK; // �� ���̽� �ø�

	return !FAILED(Device->CreateRasterizerState(&rasterizerdesc, &RasterizerState));
}

void FD3D::ReleaseDeviceAndSwapChain()
{
	if (DeviceContext)
	{
		DeviceContext->Flush(); // �����ִ� GPU ��� ����
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
