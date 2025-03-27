// RenderContext.cpp
#include "RenderContext.h"
#include "D3DContextDebugger.h"

bool FRenderContext::Initialize(std::shared_ptr<IRenderHardware> InHardware)
{
    if (!InHardware || !InHardware->IsDeviceReady())
        return false;
    RenderHardware = InHardware;
    bool result = CreateDefaultSamplerState();

    return result; 
}

void FRenderContext::Release()
{
    //디버거
    if (ContextDebugger)
    {
        delete ContextDebugger;
    }

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
	RenderHardware->BeginFrame();
}

void FRenderContext::EndFrame()
{
	RenderHardware->EndFrame();
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

bool FRenderContext::CreateDefaultSamplerState()
{
    D3D11_SAMPLER_DESC samplerDesc = {};
	//samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;  // 바이리니어 필터링, 부드러운 텍스처 표시
    samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;          // 비등방성 필터링 사용
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;     // U좌표 반복
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;     // V좌표 반복  
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;     // W좌표 반복
	samplerDesc.MinLOD = 0;                                // 최소 LOD 레벨
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;               // 최대 LOD 제한 없음
	samplerDesc.MipLODBias = 0;                           // LOD 레벨 조정 없음
    samplerDesc.MaxAnisotropy = 8;                       //비등방성 필터링 수준
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;  // 비교 샘플링 사용 안함

	HRESULT result = GetDevice()->CreateSamplerState(&samplerDesc, &DefaultSamplerState);
	return SUCCEEDED(result);
}

void FRenderContext::ValidateDeviceContextBindings()
{
    auto DeviceContext = GetDeviceContext();
#ifdef _DEBUG
    if (!bDebugValidationEnabled || !DeviceContext)
        return;

    // 최초 호출 시 디버거 생성
    if (!ContextDebugger)
    {
        ContextDebugger = new FD3DContextDebugger();
    }

    // 현재 바인딩된 리소스 상태 캡처
    ContextDebugger->CaptureBindings(DeviceContext);

    // 리소스 유효성 검사
    if (!ContextDebugger->ValidateAllBindings())
    {
        // 오류 발견 시 상세 정보 출력
        ContextDebugger->PrintBindings();

        // 사용자 지정 오류 메시지
        OutputDebugStringA("======== D3D11 디바이스 컨텍스트 유효성 검사 실패 ========\n");
        OutputDebugStringA("렌더링 파이프라인에 유효하지 않은 리소스가 바인딩되어 있습니다.\n");
        OutputDebugStringA("자세한 내용은 디버그 출력을 확인하세요.\n");
        OutputDebugStringA("===========================================================\n");

        // 디버그 모드에서 브레이크 포인트 설정(옵션)
        if (bDebugBreakOnError)
        {
            __debugbreak();
        }
    }
#endif
}