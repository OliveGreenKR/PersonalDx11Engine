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

void FRenderContext::BindVertexBuffer(ID3D11Buffer* Buffer, UINT Stride, UINT Offset)
{
    ID3D11DeviceContext* DeviceContext = GetDeviceContext();
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
    ID3D11DeviceContext* DeviceContext = GetDeviceContext();
    if (!Buffer || !DeviceContext) return;

    if (CurrentIB != Buffer)
    {
        DeviceContext->IASetIndexBuffer(Buffer, Format, 0);
        CurrentIB = Buffer;
    }
}

void FRenderContext::BindShader(ID3D11VertexShader* VS, ID3D11PixelShader* PS, ID3D11InputLayout* Layout)
{
    ID3D11DeviceContext* DeviceContext = GetDeviceContext();
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
    ID3D11DeviceContext* DeviceContext = GetDeviceContext();
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
    ID3D11DeviceContext* DeviceContext = GetDeviceContext();
    if (!SRV || !DeviceContext) return;

    DeviceContext->PSSetShaderResources(Slot, 1, &SRV);
}

void FRenderContext::BindSamplerState(UINT Slot, ID3D11SamplerState* Sampler)
{
    ID3D11DeviceContext* DeviceContext = GetDeviceContext();
    if (!Sampler || !DeviceContext) return;

    DeviceContext->PSSetSamplers(Slot, 1, &Sampler);
}

void FRenderContext::Draw(UINT VertexCount, UINT StartVertexLocation)
{
    ID3D11DeviceContext* DeviceContext = GetDeviceContext();
    if (!DeviceContext || VertexCount == 0) return;

    DeviceContext->Draw(VertexCount, StartVertexLocation);
}

void FRenderContext::DrawIndexed(UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation)
{
    ID3D11DeviceContext* DeviceContext = GetDeviceContext();
    if (!DeviceContext || IndexCount == 0) return;

    DeviceContext->DrawIndexed(IndexCount, StartIndexLocation, BaseVertexLocation);
}

void FRenderContext::CreateDefaultSamplerState()
{
    // 샘플러 상태 설명 구조체
    D3D11_SAMPLER_DESC samplerDesc = {};

    // 기본 설정으로 채우기
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR; // 선형 필터링
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;    // U 방향 텍스처 반복
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;    // V 방향 텍스처 반복
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;    // W 방향 텍스처 반복
    samplerDesc.MipLODBias = 0.0f;                       // MIP 수준 바이어스
    samplerDesc.MaxAnisotropy = 1;                       // 이방성 필터링 사용 시 최대값
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER; // 비교 함수 (쉐도우 매핑 등에서 사용)
    samplerDesc.BorderColor[0] = 0.0f;                   // 경계 색상 (clamp 모드에서 사용)
    samplerDesc.BorderColor[1] = 0.0f;
    samplerDesc.BorderColor[2] = 0.0f;
    samplerDesc.BorderColor[3] = 0.0f;
    samplerDesc.MinLOD = 0;                              // 최소 MIP 수준
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;              // 최대 MIP 수준

    // 샘플러 상태 생성
    HRESULT hr = RenderHardware->GetDevice()->CreateSamplerState(&samplerDesc, &DefaultSamplerState);
    assert(SUCCEEDED(hr));
}
