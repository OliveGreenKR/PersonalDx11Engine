#include "RenderContext.h"

void FRenderContext::PushState(IRenderState* State)
{
    if (State)
    {
        // ���� ���¸� ����
        State->Apply(DeviceContext);

        // ���� ���ÿ� �߰�
        StateStack.push(State);
    }
}

void FRenderContext::PopState()
{
    if (!StateStack.empty())
    {
        IRenderState* currentState = StateStack.top();
        StateStack.pop();

        // ���� ���� ����
        if (currentState)
        {
            currentState->Restore(DeviceContext);
        }

        // ���� ���� �ٽ� ����(���� �ֻ���)
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

void FRenderContext::BindVSConstantBuffer(UINT Slot, ID3D11Buffer* Buffer)
{
    if (!Buffer || !DeviceContext)
        return;

    // �ߺ� ���ε� ������ ���� ĳ�� üũ 
    if (VSBoundConstantBuffers[Slot] != Buffer)
    {
        // �� ���� �����͸� ����Ͽ� ��� ���� ����
        DeviceContext->VSSetConstantBuffers(Slot, 1, &Buffer);

        // ĳ�� ������Ʈ
        VSBoundConstantBuffers[Slot] = Buffer;
    }
}

void FRenderContext::BindPSConstantBuffer(UINT Slot, ID3D11Buffer* Buffer)
{
    if (!Buffer || !DeviceContext)
        return;

    // �ߺ� ���ε� ������ ���� ĳ�� üũ 
    if (PSBoundConstantBuffers[Slot] != Buffer)
    {
        // �� ���� �����͸� ����Ͽ� ��� ���� ����
        DeviceContext->PSSetConstantBuffers(Slot, 1, &Buffer);

        // ĳ�� ������Ʈ
        PSBoundConstantBuffers[Slot] = Buffer;
    }
}

void FRenderContext::UpdateVSConstantBuffer(UINT StartSlot, ID3D11Buffer* Buffer, const void* Data, size_t DataSize)
{
    if (!DeviceContext || (DataSize % 16) != 0)
    {
        return;
    }

    D3D11_MAPPED_SUBRESOURCE MappedResource;
    HRESULT result = DeviceContext->Map(VSBoundConstantBuffers[StartSlot], 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);

    if (FAILED(result))
        return;

    //from CPU to GPU memory
    memcpy(MappedResource.pData, &Buffer, DataSize);
    DeviceContext->Unmap(VSBoundConstantBuffers[StartSlot], 0);

    //constant buffer bind
    DeviceContext->VSSetConstantBuffers(StartSlot, 1, &VSBoundConstantBuffers[StartSlot]);
}

void FRenderContext::UpdatePSConstantBuffer(UINT StartSlot, ID3D11Buffer* Buffer, const void* Data, size_t DataSize)
{
    if (!DeviceContext || (DataSize % 16) != 0)
    {
        return;
    }

    D3D11_MAPPED_SUBRESOURCE MappedResource;
    HRESULT result = DeviceContext->Map(PSBoundConstantBuffers[StartSlot], 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);

    if (FAILED(result))
        return;

    //from CPU to GPU memory
    memcpy(MappedResource.pData, &Buffer, DataSize);
    DeviceContext->Unmap(PSBoundConstantBuffers[StartSlot], 0);

    //constant buffer bind
    DeviceContext->VSSetConstantBuffers(StartSlot, 1, &PSBoundConstantBuffers[StartSlot]);
}

void FRenderContext::BindVertexBuffer(ID3D11Buffer* Buffer, UINT Stride, UINT Offset)
{
    if (!Buffer)
        return;

    // ���� ĳ�� �߰� (����ȭ)
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
    if (!DeviceContext)
        return;

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

void FRenderContext::BindShaderResource(UINT Slot, ID3D11ShaderResourceView* Texture)
{
    if (!DeviceContext || !Texture)
    {
        return;
    }
    DeviceContext->PSSetShaderResources(Slot, 1, &Texture);
}

void FRenderContext::BindSampler(UINT Slot, ID3D11SamplerState* SamplerState)
{
    if (!DeviceContext || !SamplerState)
    {
        return;
    }
    DeviceContext->PSSetSamplers(Slot, 1, &SamplerState);
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