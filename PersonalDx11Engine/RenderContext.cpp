// RenderContext.cpp
#include "RenderContext.h"

bool FRenderContext::Initialize(std::shared_ptr<IRenderHardware> InHardware)
{
    if (!InHardware || !InHardware->IsDeviceReady())
        return false;
    RenderHardware = InHardware;
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

