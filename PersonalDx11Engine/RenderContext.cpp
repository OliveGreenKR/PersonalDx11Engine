// RenderContext.cpp
#include "RenderContext.h"
#include "D3DContextDebugger.h"

bool FRenderContext::Initialize(std::shared_ptr<IRenderHardware> InHardware)
{
    if (!InHardware || !InHardware->IsDeviceReady())
        return false;
    RenderHardware = InHardware;

    return true; 
}

void FRenderContext::Release()
{
    //디버거
    if (ContextDebugger)
    {
        delete ContextDebugger;
    }

	//캐시 해제
	CurrentVB = nullptr;
	CurrentIB = nullptr;
	CurrentVS = nullptr;
	CurrentPS = nullptr;
	CurrentLayout = nullptr;

    if (RenderHardware)
    {
        RenderHardware = nullptr;
    }
}

void FRenderContext::BeginFrame()
{
	RenderHardware->BeginFrame();
}

void FRenderContext::EndFrame()
{
    ValidateDeviceContextBindings();
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

void FRenderContext::DrawRenderData(const IRenderData* InData)
{
    // 1.  버텍스 버퍼 바인딩
    auto VertexBuffer = InData->GetVertexBuffer();
    auto Stride = InData->GetStride();
    auto Offset = InData->GetOffset();
    if (VertexBuffer)
    {
        this->BindVertexBuffer(VertexBuffer, Stride, Offset);
    }

    auto IndexBuffer = InData->GetIndexBuffer();
    if (IndexBuffer)
    {
        this->BindIndexBuffer(IndexBuffer);
    }

    // 2. 상수 버퍼 바인딩 (Vertex Shader)
    size_t VSConstantBufferCount = InData->GetVSConstantBufferCount();
    for (size_t i = 0; i < VSConstantBufferCount; ++i)
    {
        uint32_t Slot;
        ID3D11Buffer* Buffer;
        void* Data;
        size_t DataSize;
        InData->GetVSConstantBufferData(i, Slot, Buffer, Data, DataSize);
        this->BindConstantBuffer(Slot, Buffer, Data, DataSize, true);
    }

    // 3. 상수 버퍼 바인딩 (Pixel Shader)
    size_t PSConstantBufferCount = InData->GetPSConstantBufferCount();
    for (size_t i = 0; i < PSConstantBufferCount; ++i)
    {
        uint32_t Slot;
        ID3D11Buffer* Buffer;
        void* Data;
        size_t DataSize;
        InData->GetPSConstantBufferData(i, Slot, Buffer, Data, DataSize);
        this->BindConstantBuffer(Slot, Buffer, Data, DataSize, false);
    }

    // 4. 텍스처 및 쉐이더 리소스
    size_t TextureCount = InData->GetTextureCount();
    for (size_t i = 0; i < TextureCount; ++i)
    {
        uint32_t Slot;
        ID3D11ShaderResourceView* SRV;
        InData->GetTextureData(i, Slot, SRV);
        this->BindShaderResource(Slot, SRV);
    }

    // 5. 샘플러 (인터페이스에 정의되지 않았으므로 유지 불가 - 주석 처리)
    /*
    for (const auto& Samp : Samplers)
    {
        this->BindSamplerState(Samp.Slot, Samp.Sampler);
    }
    */

    // 6. 드로우 콜 실행
    uint32_t IndexCount = InData->GetIndexCount();
    uint32_t VertexCount = InData->GetVertexCount();
    uint32_t StartIndex = InData->GetStartIndex();
    uint32_t BaseVertex = InData->GetBaseVertexLocatioan();
    if (IndexCount > 0)
    {
        this->DrawIndexed(IndexCount, StartIndex, BaseVertex);
    }
    else if (VertexCount > 0)
    {
        this->Draw(VertexCount, BaseVertex);
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

void FRenderContext::PrintCurrentBindins()
{
    auto DeviceContext = GetDeviceContext();
    if (!DeviceContext)
        return;
#ifdef _DEBUG
    // 최초 호출 시 디버거 생성
    if (!ContextDebugger)
    {
        ContextDebugger = new FD3DContextDebugger();
    }

    // 현재 바인딩된 리소스 상태 캡처
    ContextDebugger->CaptureBindings(DeviceContext);
    // 오류 발견 시 상세 정보 출력
    ContextDebugger->PrintBindings();
#endif
}

