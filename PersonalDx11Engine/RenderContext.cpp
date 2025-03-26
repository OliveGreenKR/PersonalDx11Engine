// RenderContext.cpp
#include "RenderContext.h"

bool FRenderContext::Initialize(std::shared_ptr<IRenderHardware> InHardware)
{
    if (!InHardware || !InHardware->IsDeviceReady())
        return false;

    RenderHardware = InHardware;
	FrameBufferRTV = RenderHardware->GetRenderTargetView();

    CreateDefaultSamplerState();

	bool result =
		CreateDefaultSamplerState() &&
		CreateDefaultRasterizerState() &&
		CreateDpethStencilBuffer() &&
		CreateDepthStencilState() &&
		CreateDepthStencillView() &&
		CreateBlendState();

	RenderHardware->GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	RenderHardware->GetDeviceContext()->OMSetRenderTargets(1, &FrameBufferRTV, DepthStencilView);
	RenderHardware->GetDeviceContext()->OMSetBlendState(BlendState, nullptr, 0xffffffff);
	RenderHardware->GetDeviceContext()->OMSetDepthStencilState(DepthStencilState, 1);

    return true; // 성공 시 true 반환
}

void FRenderContext::Release()
{
    //기본 샘플러 
    DefaultSamplerState->Release();
    DefaultSamplerState = nullptr;

	//캐시 해제
	CurrentVB = nullptr;
	CurrentIB = nullptr;
	CurrentVS = nullptr;
	CurrentPS = nullptr;
	CurrentLayout = nullptr;
   
    while (!StateStack.empty())
    {
        StateStack.pop();
    }



    if (RenderHardware)
    {
        RenderHardware = nullptr;
    }
}

void FRenderContext::PushState(IRenderState* State)
{
    ID3D11DeviceContext* DeviceContext = GetDeviceContext();
    if (!State || !DeviceContext) return;

    // 상태 스택에 푸시
    State->Apply(DeviceContext);
    StateStack.push(State);
}

void FRenderContext::PopState()
{
    ID3D11DeviceContext* DeviceContext = GetDeviceContext();
    if (StateStack.empty() || !DeviceContext) return;

    IRenderState* CurrentState = StateStack.top();
    StateStack.pop();

    // 현재 상태 복원
    if (CurrentState)
    {
        CurrentState->Restore(DeviceContext);
    }

    // 이전 상태가 있다면 다시 적용
    if (!StateStack.empty())
    {
        StateStack.top()->Apply(DeviceContext);
    }
}

void FRenderContext::BeginFrame()
{
	RenderHardware->GetDeviceContext()->ClearRenderTargetView(RenderHardware->GetRenderTargetView(), ClearColor);
	RenderHardware->GetDeviceContext()->ClearDepthStencilView(DepthStencilView,
										 D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	//OutputMerge
	

	//Rasterizer
	RenderHardware->GetDeviceContext()->RSSetViewports(1, &ViewportInfo);
	RenderHardware->GetDeviceContext()->RSSetState(RasterizerState);

	//Input Assembly
	
}

void FRenderContext::BindVertexBuffer(ID3D11Buffer* Buffer, UINT Stride, UINT Offset)
{
    ID3D11DeviceContext* DeviceContext = GetDeviceContext();
    if (!Buffer || !DeviceContext) return;

    // 현재 바인딩된 버퍼와 같은지 확인하여 중복 바인딩 방지
    if (CurrentVB != Buffer)
    {
        RenderHardware->GetDeviceContext()->IASetVertexBuffers(0, 1, &Buffer, &Stride, &Offset);
        CurrentVB = Buffer;
    }
}

void FRenderContext::BindIndexBuffer(ID3D11Buffer* Buffer, DXGI_FORMAT Format)
{
    ID3D11DeviceContext* DeviceContext = GetDeviceContext();
    if (!Buffer || !DeviceContext) return;

    if (CurrentIB != Buffer)
    {
        RenderHardware->GetDeviceContext()->IASetIndexBuffer(Buffer, Format, 0);
        CurrentIB = Buffer;
    }
}

void FRenderContext::BindShader(ID3D11VertexShader* VS, ID3D11PixelShader* PS, ID3D11InputLayout* Layout)
{
    ID3D11DeviceContext* DeviceContext = GetDeviceContext();
    if (!DeviceContext) return;

    if (VS && CurrentVS != VS)
    {
        RenderHardware->GetDeviceContext()->VSSetShader(VS, nullptr, 0);
        CurrentVS = VS;
    }

    if (PS && CurrentPS != PS)
    {
        RenderHardware->GetDeviceContext()->PSSetShader(PS, nullptr, 0);
        CurrentPS = PS;
    }

    if (Layout && CurrentLayout != Layout)
    {
        RenderHardware->GetDeviceContext()->IASetInputLayout(Layout);
        CurrentLayout = Layout;
    }
}

void FRenderContext::BindConstantBuffer(UINT Slot, ID3D11Buffer* Buffer, const void* Data, size_t Size, bool IsVertexShader)
{
    ID3D11DeviceContext* DeviceContext = GetDeviceContext();
    if (!Buffer || !DeviceContext || !Data || Size == 0) return;

    D3D11_MAPPED_SUBRESOURCE MappedResource;
    HRESULT Result = RenderHardware->GetDeviceContext()->Map(Buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);

    if (SUCCEEDED(Result))
    {
        memcpy(MappedResource.pData, Data, Size);
        RenderHardware->GetDeviceContext()->Unmap(Buffer, 0);

        if (IsVertexShader)
        {
            RenderHardware->GetDeviceContext()->VSSetConstantBuffers(Slot, 1, &Buffer);
        }
        else
        {
            RenderHardware->GetDeviceContext()->PSSetConstantBuffers(Slot, 1, &Buffer);
        }
    }
}

void FRenderContext::BindShaderResource(UINT Slot, ID3D11ShaderResourceView* SRV)
{
    ID3D11DeviceContext* DeviceContext = GetDeviceContext();
    if (!SRV || !DeviceContext) return;

    RenderHardware->GetDeviceContext()->PSSetShaderResources(Slot, 1, &SRV);
}

void FRenderContext::BindSamplerState(UINT Slot, ID3D11SamplerState* Sampler)
{
    ID3D11DeviceContext* DeviceContext = GetDeviceContext();
    if (!Sampler || !DeviceContext) return;

    RenderHardware->GetDeviceContext()->PSSetSamplers(Slot, 1, &Sampler);
}

void FRenderContext::Draw(UINT VertexCount, UINT StartVertexLocation)
{
    ID3D11DeviceContext* DeviceContext = GetDeviceContext();
    if (!DeviceContext || VertexCount == 0) return;

    RenderHardware->GetDeviceContext()->Draw(VertexCount, StartVertexLocation);
}

void FRenderContext::DrawIndexed(UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation)
{
    ID3D11DeviceContext* DeviceContext = GetDeviceContext();
    if (!DeviceContext || IndexCount == 0) return;

    RenderHardware->GetDeviceContext()->DrawIndexed(IndexCount, StartIndexLocation, BaseVertexLocation);
}

bool FRenderContext::CreateDefaultRasterizerState()
{
	D3D11_RASTERIZER_DESC rasterizerdesc = {};
	rasterizerdesc.FillMode = D3D11_FILL_SOLID; // 채우기 모드
	rasterizerdesc.CullMode = D3D11_CULL_BACK; // 백 페이스 컬링

	return SUCCEEDED(RenderHardware->GetDevice()->CreateRasterizerState(&rasterizerdesc, &RasterizerState));
}

bool FRenderContext::CreateDpethStencilBuffer()
{

	auto ViewportInfo = RenderHardware->GetViewPort();

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

	HRESULT hr = RenderHardware->GetDevice()->CreateTexture2D(&depthBufferDesc, nullptr, &DepthStencilBuffer);
	return SUCCEEDED(hr);
}

bool FRenderContext::CreateDepthStencilState()
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

	HRESULT hr = RenderHardware->GetDevice()->CreateDepthStencilState(&depthStencilDesc, &DepthStencilState);
	return SUCCEEDED(hr);
}

bool FRenderContext::CreateDepthStencillView()
{
	D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {};
	depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	depthStencilViewDesc.Texture2D.MipSlice = 0;

	HRESULT hr = RenderHardware->GetDevice()->CreateDepthStencilView(DepthStencilBuffer, &depthStencilViewDesc, &DepthStencilView);
	return SUCCEEDED(hr);
}

bool FRenderContext::CreateBlendState()
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

	return SUCCEEDED(RenderHardware->GetDevice()->CreateBlendState(&blendDesc, &BlendState));
}

bool FRenderContext::CreateDefaultSamplerState()
{
	D3D11_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;  // 바이리니어 필터링, 부드러운 텍스처 표시
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;     // U좌표 반복
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;     // V좌표 반복  
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;     // W좌표 반복
	samplerDesc.MinLOD = 0;                                // 최소 LOD 레벨
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;               // 최대 LOD 제한 없음
	samplerDesc.MipLODBias = 0;                           // LOD 레벨 조정 없음
	samplerDesc.MaxAnisotropy = 1;                        // 비등방성 필터링 사용 안함
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;  // 비교 샘플링 사용 안함

	HRESULT result = GetDevice()->CreateSamplerState(&samplerDesc, &DefaultSamplerState);
	return SUCCEEDED(result);
}

void FRenderContext::ReleaseRasterizerState()
{
	if (RasterizerState)
	{
		RasterizerState->Release();
		RasterizerState = nullptr;
	}
}

void FRenderContext::ReleaseDepthStencil()
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
