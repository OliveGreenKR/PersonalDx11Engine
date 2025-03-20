#include "RenderContext.h"

void FRenderContext::PushState(IRenderState* State)
{
    if (State)
    {
        // 현재 상태를 적용
        State->Apply(DeviceContext);

        // 상태 스택에 추가
        StateStack.push(State);
    }
}

void FRenderContext::PopState()
{
    if (!StateStack.empty())
    {
        IRenderState* currentState = StateStack.top();
        StateStack.pop();

        // 현재 상태 복원
        if (currentState)
        {
            currentState->Restore(DeviceContext);
        }

        // 이전 상태 다시 적용(스택 최상위)
        if (!StateStack.empty())
        {
            IRenderState* previousState = StateStack.top();
            if (previousState)
            {
                previousState->Apply(DeviceContext);
            }
        }
    }
}

void FRenderContext::BindVertexBuffer(ID3D11Buffer* Buffer, UINT Stride, UINT Offset)
{
    if (!Buffer)
        return;

    // 버퍼 캐싱 추가 (최적화)
    std::string bufferKey = "VB_" + std::to_string(reinterpret_cast<uintptr_t>(Buffer));
    auto it = BoundBuffers.find(bufferKey);

    if (it == BoundBuffers.end() || it->second != Buffer)
    {
        DeviceContext->IASetVertexBuffers(0, 1, &Buffer, &Stride, &Offset);
        BoundBuffers[bufferKey] = Buffer;
    }
}

void FRenderContext::BindIndexBuffer(ID3D11Buffer* Buffer, DXGI_FORMAT Format)
{
    if (!Buffer)
        return;

    std::string bufferKey = "IB_" + std::to_string(reinterpret_cast<uintptr_t>(Buffer));
    auto it = BoundBuffers.find(bufferKey);

    if (it == BoundBuffers.end() || it->second != Buffer)
    {
        DeviceContext->IASetIndexBuffer(Buffer, Format, 0);
        BoundBuffers[bufferKey] = Buffer;
    }
}


void FRenderContext::BindShader(ID3D11VertexShader* VS, ID3D11PixelShader* PS)
{
    if (VS)
    {
        std::string key = "VS_" + std::to_string(reinterpret_cast<uintptr_t>(VS));
        auto it = VertexShaders.find(key);

        if (it == VertexShaders.end() || it->second != VS)
        {
            DeviceContext->VSSetShader(VS, nullptr, 0);
            CurrentVS = VS;
            VertexShaders[key] = VS;
        }
    }

    if (PS)
    {
        std::string key = "PS_" + std::to_string(reinterpret_cast<uintptr_t>(PS));
        auto it = PixelShaders.find(key);

        if (it == PixelShaders.end() || it->second != PS)
        {
            DeviceContext->PSSetShader(PS, nullptr, 0);
            CurrentPS = PS;
            PixelShaders[key] = PS;
        }
    }
}

void FRenderContext::Draw(UINT VertexCount, UINT StartVertexLocation)
{
    if (VertexCount > 0)
    {
        DeviceContext->Draw(VertexCount, StartVertexLocation);
    }
}

void FRenderContext::DrawIndexed(UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation)
{
    if (IndexCount > 0)
    {
        DeviceContext->DrawIndexed(IndexCount, StartIndexLocation, BaseVertexLocation);
    }
}