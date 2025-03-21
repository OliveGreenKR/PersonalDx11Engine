// RenderContext.cpp
#include "RenderContext.h"

void FRenderContext::PushState(IRenderState* State)
{
    if (!State || !DeviceContext) return;

    // 상태 스택에 푸시
    State->Apply(DeviceContext);
    StateStack.push(State);
}

void FRenderContext::PopState()
{
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

void FRenderContext::BindVertexBuffer(ID3D11Buffer* Buffer, UINT Stride, UINT Offset)
{
    if (!Buffer || !DeviceContext) return;

    // 현재 바인딩된 버퍼와 같은지 확인하여 중복 바인딩 방지
    if (CurrentVB != Buffer)
    {
        DeviceContext->IASetVertexBuffers(0, 1, &Buffer, &Stride, &Offset);
        CurrentVB = Buffer;
    }
}

void FRenderContext::BindIndexBuffer(ID3D11Buffer* Buffer, DXGI_FORMAT Format)
{
    if (!Buffer || !DeviceContext) return;

    if (CurrentIB != Buffer)
    {
        DeviceContext->IASetIndexBuffer(Buffer, Format, 0);
        CurrentIB = Buffer;
    }
}

void FRenderContext::BindShader(ID3D11VertexShader* VS, ID3D11PixelShader* PS, ID3D11InputLayout* Layout)
{
    if (!DeviceContext) return;

    if (VS && CurrentVS != VS)
    {
        DeviceContext->VSSetShader(VS, nullptr, 0);
        CurrentVS = VS;
    }

    if (PS && CurrentPS != PS)
    {
        DeviceContext->PSSetShader(PS, nullptr, 0);
        CurrentPS = PS;
    }

    if (Layout && CurrentLayout != Layout)
    {
        DeviceContext->IASetInputLayout(Layout);
        CurrentLayout = Layout;
    }
}

void FRenderContext::BindConstantBuffer(UINT Slot, ID3D11Buffer* Buffer, const void* Data, size_t Size, bool IsVertexShader)
{
    if (!Buffer || !DeviceContext || !Data || Size == 0) return;

    D3D11_MAPPED_SUBRESOURCE MappedResource;
    HRESULT Result = DeviceContext->Map(Buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);

    if (SUCCEEDED(Result))
    {
        memcpy(MappedResource.pData, Data, Size);
        DeviceContext->Unmap(Buffer, 0);

        if (IsVertexShader)
        {
            DeviceContext->VSSetConstantBuffers(Slot, 1, &Buffer);
        }
        else
        {
            DeviceContext->PSSetConstantBuffers(Slot, 1, &Buffer);
        }
    }
}

void FRenderContext::BindShaderResource(UINT Slot, ID3D11ShaderResourceView* SRV)
{
    if (!SRV || !DeviceContext) return;

    DeviceContext->PSSetShaderResources(Slot, 1, &SRV);
}

void FRenderContext::BindSamplerState(UINT Slot, ID3D11SamplerState* Sampler)
{
    if (!Sampler || !DeviceContext) return;

    DeviceContext->PSSetSamplers(Slot, 1, &Sampler);
}

void FRenderContext::Draw(UINT VertexCount, UINT StartVertexLocation)
{
    if (!DeviceContext || VertexCount == 0) return;

    DeviceContext->Draw(VertexCount, StartVertexLocation);
}

void FRenderContext::DrawIndexed(UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation)
{
    if (!DeviceContext || IndexCount == 0) return;

    DeviceContext->DrawIndexed(IndexCount, StartIndexLocation, BaseVertexLocation);
}