// D3DContextDebugger.cpp
#include "D3DContextDebugger.h"
#include "Debug.h" // 로깅을 위한 자체 헤더 가정

void FD3DContextDebugger::CaptureBindings(ID3D11DeviceContext* DeviceContext)
{
    if (!DeviceContext)
    {
        LOG("Error: NULL DeviceContext passed to CaptureBindings");
        return;
    }


    // 1. 입력 어셈블리 스테이지
    // 정점 버퍼 캡처
    ID3D11Buffer* vBuffers[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT] = {};
    UINT strides[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT] = {};
    UINT offsets[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT] = {};
    DeviceContext->IAGetVertexBuffers(0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT, vBuffers, strides, offsets);

    VertexBuffers.clear();
    for (UINT i = 0; i < D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT; i++)
    {
        if (vBuffers[i])
        {
            FResourceBinding binding;
            binding.Type = "VertexBuffer";
            binding.Name = "Slot_" + std::to_string(i);
            binding.Slot = i;
            binding.Resource = vBuffers[i];
            binding.bIsValid = (vBuffers[i] != nullptr);
            VertexBuffers.push_back(binding);
            vBuffers[i]->Release();
        }
    }

    // 인덱스 버퍼 캡처
    ID3D11Buffer* indexBuffer = nullptr;
    DXGI_FORMAT format;
    UINT offset;
    DeviceContext->IAGetIndexBuffer(&indexBuffer, &format, &offset);

    IndexBuffers.clear();
    if (indexBuffer)
    {
        FResourceBinding binding;
        binding.Type = "IndexBuffer";
        binding.Name = "Main";
        binding.Slot = 0;
        binding.Resource = indexBuffer;
        binding.bIsValid = (indexBuffer != nullptr);
        IndexBuffers.push_back(binding);
        indexBuffer->Release();
    }

    // 2. 정점 쉐이더 스테이지
    ID3D11VertexShader* vs = nullptr;
    DeviceContext->VSGetShader(&vs, nullptr, nullptr);
    VertexShader.Type = "VertexShader";
    VertexShader.Name = "Main";
    VertexShader.Resource = vs;
    VertexShader.bIsValid = (vs != nullptr);
    if (vs) 
        vs->Release();

    // 입력 레이아웃 캡처
    ID3D11InputLayout* layout = nullptr;
    DeviceContext->IAGetInputLayout(&layout);
    InputLayout.Type = "InputLayout";
    InputLayout.Name = "Main";
    InputLayout.Resource = layout;
    InputLayout.bIsValid = (layout != nullptr);
    if (layout) 
        layout->Release();

    // 정점 쉐이더 상수 버퍼 캡처
    ID3D11Buffer* vsConstBuffers[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT] = {};
    DeviceContext->VSGetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, vsConstBuffers);

    ConstantBuffersVS.clear();
    for (UINT i = 0; i < D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT; i++)
    {
        if (vsConstBuffers[i])
        {
            FResourceBinding binding;
            binding.Type = "VSConstantBuffer";
            binding.Name = "Slot_" + std::to_string(i);
            binding.Slot = i;
            binding.Resource = vsConstBuffers[i];
            binding.bIsValid = (vsConstBuffers[i] != nullptr);
            ConstantBuffersVS.push_back(binding);
            vsConstBuffers[i]->Release();
        }
    }

    // 3. 픽셀 쉐이더 스테이지
    ID3D11PixelShader* ps = nullptr;
    DeviceContext->PSGetShader(&ps, nullptr, nullptr);
    PixelShader.Type = "PixelShader";
    PixelShader.Name = "Main";
    PixelShader.Resource = ps;
    PixelShader.bIsValid = (ps != nullptr);
    if (ps) 
        ps->Release();

    // 픽셀 쉐이더 상수 버퍼 캡처
    ID3D11Buffer* psConstBuffers[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT] = {};
    DeviceContext->PSGetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, psConstBuffers);

    ConstantBuffersPS.clear();
    for (UINT i = 0; i < D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT; i++)
    {
        if (psConstBuffers[i])
        {
            FResourceBinding binding;
            binding.Type = "PSConstantBuffer";
            binding.Name = "Slot_" + std::to_string(i);
            binding.Slot = i;
            binding.Resource = psConstBuffers[i];
            binding.bIsValid = (psConstBuffers[i] != nullptr);
            ConstantBuffersPS.push_back(binding);
            psConstBuffers[i]->Release();
        }
    }

    // 쉐이더 리소스 뷰 캡처
    ID3D11ShaderResourceView* srvs[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT] = {};
    DeviceContext->PSGetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, srvs);

    ShaderResources.clear();
    for (UINT i = 0; i < D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT; i++)
    {
        if (srvs[i])
        {
            FResourceBinding binding;
            binding.Type = "ShaderResourceView";
            binding.Name = "Slot_" + std::to_string(i);
            binding.Slot = i;
            binding.Resource = srvs[i];
            binding.bIsValid = (srvs[i] != nullptr);
            ShaderResources.push_back(binding);
            srvs[i]->Release();
        }
    }

    // 샘플러 상태 캡처
    ID3D11SamplerState* samplers[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT] = {};
    DeviceContext->PSGetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, samplers);

    Samplers.clear();
    for (UINT i = 0; i < D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT; i++)
    {
        if (samplers[i])
        {
            FResourceBinding binding;
            binding.Type = "SamplerState";
            binding.Name = "Slot_" + std::to_string(i);
            binding.Slot = i;
            binding.Resource = samplers[i];
            binding.bIsValid = (samplers[i] != nullptr);
            Samplers.push_back(binding);
            samplers[i]->Release();
        }
    }

    // 4. 출력 병합 스테이지
    // 렌더 타겟 뷰 캡처
    ID3D11RenderTargetView* rtvs[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
    ID3D11DepthStencilView* dsv = nullptr;
    DeviceContext->OMGetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, rtvs, &dsv);

    for (UINT i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
    {
        RenderTargets[i].Type = "RenderTargetView";
        RenderTargets[i].Name = "Slot_" + std::to_string(i);
        RenderTargets[i].Slot = i;
        RenderTargets[i].Resource = rtvs[i];
        RenderTargets[i].bIsValid = (rtvs[i] != nullptr);
        if (rtvs[i]) rtvs[i]->Release();
    }

    DepthStencilView.Type = "DepthStencilView";
    DepthStencilView.Name = "Main";
    DepthStencilView.Resource = dsv;
    DepthStencilView.bIsValid = (dsv != nullptr);
    if (dsv) 
        dsv->Release();

    // 블렌드 상태 캡처
    ID3D11BlendState* blendState = nullptr;
    FLOAT blendFactor[4];
    UINT sampleMask;
    DeviceContext->OMGetBlendState(&blendState, blendFactor, &sampleMask);

    BlendState.Type = "BlendState";
    BlendState.Name = "Main";
    BlendState.Resource = blendState;
    BlendState.bIsValid = (blendState != nullptr);
    if (blendState) 
        blendState->Release();

    // 5. 래스터라이저 스테이지
    // 뷰포트 설정 캡처
    D3D11_VIEWPORT viewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
    UINT numViewports = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    DeviceContext->RSGetViewports(&numViewports, viewports);

    LOG("--- Viewport Settings ---");
    for (UINT i = 0; i < numViewports; i++)
    {
        LOG("Viewport %u: Width=%.2f, Height=%.2f, TopLeft=(%.2f, %.2f), MinDepth=%.2f, MaxDepth=%.2f",
            i, viewports[i].Width, viewports[i].Height,
            viewports[i].TopLeftX, viewports[i].TopLeftY,
            viewports[i].MinDepth, viewports[i].MaxDepth);
    }
    //래스터라이저
    ID3D11RasterizerState* rasterizerState = nullptr;
    DeviceContext->RSGetState(&rasterizerState);

    RasterizerState.Type = "RasterizerState";
    RasterizerState.Name = "Main";
    RasterizerState.Resource = rasterizerState;
    RasterizerState.bIsValid = (rasterizerState != nullptr);
    if (rasterizerState) rasterizerState->Release();

    // 6. 깊이 스텐실 상태 캡처
    ID3D11DepthStencilState* depthStencilState = nullptr;
    UINT stencilRef;
    DeviceContext->OMGetDepthStencilState(&depthStencilState, &stencilRef);

    DepthStencilState.Type = "DepthStencilState";
    DepthStencilState.Name = "Main";
    DepthStencilState.Resource = depthStencilState;
    DepthStencilState.bIsValid = (depthStencilState != nullptr);
    if (depthStencilState) 
        depthStencilState->Release();
}

bool FD3DContextDebugger::ValidateAllBindings() const
{
    bool bIsValid = true;

    // 필수적인 리소스 검증 (렌더링에 반드시 필요한 것들)
    if (!ValidateVertexBuffers())
    {
        LOG("Error: Invalid vertex buffer binding");
        bIsValid = false;
    }

    if (!ValidateShaders())
    {
        LOG("Error: Invalid shader binding");
        bIsValid = false;
    }

    if (!ValidateRenderTargets())
    {
        LOG("Error: Invalid render target binding");
        bIsValid = false;
    }

    // 옵셔널 리소스 검증 (있을 수도, 없을 수도 있는 것들)
    if (!ValidateIndexBuffers() && !IndexBuffers.empty())
    {
        LOG("Warning: Invalid index buffer binding");
    }

    if (!ValidateConstantBuffers() && (!ConstantBuffersVS.empty() || !ConstantBuffersPS.empty()))
    {
        LOG("Warning: Invalid constant buffer binding");
    }

    if (!ValidateShaderResources() && !ShaderResources.empty())
    {
        LOG("Warning: Invalid shader resource binding");
    }

    if (!ValidateSamplers() && !Samplers.empty())
    {
        LOG("Warning: Invalid sampler binding");
    }

    if (!ValidateRasterizerState())
    {
        LOG("Warning: Invalid rasterizer state");
    }

    if (!ValidateBlendState())
    {
        LOG("Warning: Invalid blend state");
    }

    if (!ValidateDepthStencilState())
    {
        LOG("Warning: Invalid depth stencil state");
    }

    return bIsValid;
}

void FD3DContextDebugger::PrintBindings() const
{
    LOG("======== D3D11 Device Context Bindings ========");

    // 정점 버퍼 출력
    LOG("--- Vertex Buffers ---");
    for (const auto& vb : VertexBuffers)
    {
        LOG("%s (Slot %u): %s", vb.Name.c_str(), vb.Slot, vb.bIsValid ? "Valid" : "INVALID");
    }

    // 인덱스 버퍼 출력
    LOG("--- Index Buffers ---");
    for (const auto& ib : IndexBuffers)
    {
        LOG("%s: %s", ib.Name.c_str(), ib.bIsValid ? "Valid" : "INVALID");
    }

    // 쉐이더 출력
    LOG("--- Shaders ---");
    LOG("VertexShader: %s", VertexShader.bIsValid ? "Valid" : "INVALID");
    LOG("PixelShader: %s", PixelShader.bIsValid ? "Valid" : "INVALID");
    LOG("InputLayout: %s", InputLayout.bIsValid ? "Valid" : "INVALID");

    // 상수 버퍼 출력
    LOG("--- Constant Buffers (VS) ---");
    for (const auto& cb : ConstantBuffersVS)
    {
        LOG("%s (Slot %u): %s", cb.Name.c_str(), cb.Slot, cb.bIsValid ? "Valid" : "INVALID");
    }

    LOG("--- Constant Buffers (PS) ---");
    for (const auto& cb : ConstantBuffersPS)
    {
        LOG("%s (Slot %u): %s", cb.Name.c_str(), cb.Slot, cb.bIsValid ? "Valid" : "INVALID");
    }

    // 쉐이더 리소스 출력
    LOG("--- Shader Resources ---");
    for (const auto& srv : ShaderResources)
    {
        LOG("%s (Slot %u): %s", srv.Name.c_str(), srv.Slot, srv.bIsValid ? "Valid" : "INVALID");
    }

    // 샘플러 출력
    LOG("--- Samplers ---");
    for (const auto& sampler : Samplers)
    {
        LOG("%s (Slot %u): %s", sampler.Name.c_str(), sampler.Slot, sampler.bIsValid ? "Valid" : "INVALID");
    }

    // 렌더 타겟 출력
    LOG("--- Render Targets ---");
    for (UINT i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
    {
        if (RenderTargets[i].Resource)
        {
            LOG("%s (Slot %u): %s", RenderTargets[i].Name.c_str(), i, RenderTargets[i].bIsValid ? "Valid" : "INVALID");
        }
    }

    LOG("DepthStencilView: %s", DepthStencilView.bIsValid ? "Valid" : "INVALID");

    // 상태 객체 출력
    LOG("--- States ---");
    LOG("RasterizerState: %s", RasterizerState.bIsValid ? "Valid" : "INVALID");
    LOG("BlendState: %s", BlendState.bIsValid ? "Valid" : "INVALID");
    LOG("DepthStencilState: %s", DepthStencilState.bIsValid ? "Valid" : "INVALID");

    LOG("================================================");
}

bool FD3DContextDebugger::ValidateVertexBuffers() const
{
    for (const auto& vb : VertexBuffers)
    {
        if (!vb.bIsValid) return false;
    }
    return !VertexBuffers.empty();
}

bool FD3DContextDebugger::ValidateIndexBuffers() const
{
    for (const auto& ib : IndexBuffers)
    {
        if (!ib.bIsValid) return false;
    }
    return true; // 인덱스 버퍼는 없을 수도 있음
}

bool FD3DContextDebugger::ValidateShaders() const
{
    return VertexShader.bIsValid && PixelShader.bIsValid && InputLayout.bIsValid;
}

bool FD3DContextDebugger::ValidateConstantBuffers() const
{
    for (const auto& cb : ConstantBuffersVS)
    {
        if (!cb.bIsValid) return false;
    }
    for (const auto& cb : ConstantBuffersPS)
    {
        if (!cb.bIsValid) return false;
    }
    return true;
}

bool FD3DContextDebugger::ValidateShaderResources() const
{
    for (const auto& srv : ShaderResources)
    {
        if (!srv.bIsValid) return false;
    }
    return true;
}

bool FD3DContextDebugger::ValidateSamplers() const
{
    for (const auto& sampler : Samplers)
    {
        if (!sampler.bIsValid) return false;
    }
    return true;
}

bool FD3DContextDebugger::ValidateRenderTargets() const
{
    bool hasValidRenderTarget = false;
    for (UINT i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
    {
        if (RenderTargets[i].Resource)
        {
            hasValidRenderTarget = RenderTargets[i].bIsValid;
            if (!hasValidRenderTarget) 
                return false;

            // 추가 검사: 렌더 타겟 크기 확인
            ID3D11RenderTargetView* rtv = static_cast<ID3D11RenderTargetView*>(RenderTargets[i].Resource);
            ID3D11Resource* resource = nullptr;
            rtv->GetResource(&resource);

            if (resource)
            {
                ID3D11Texture2D* texture = nullptr;
                if (SUCCEEDED(resource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&texture)))
                {
                    D3D11_TEXTURE2D_DESC desc;
                    texture->GetDesc(&desc);
                    LOG("RenderTarget %d dimensions: %ux%u", i, desc.Width, desc.Height);
                    texture->Release();
                }
                resource->Release();
            }
        }
    }
    return hasValidRenderTarget && DepthStencilView.bIsValid;
}

bool FD3DContextDebugger::ValidateRasterizerState() const
{
    return RasterizerState.bIsValid;
}

bool FD3DContextDebugger::ValidateBlendState() const
{
    return BlendState.bIsValid;
}

bool FD3DContextDebugger::ValidateDepthStencilState() const
{
    return DepthStencilState.bIsValid;
}

bool FD3DContextDebugger::InspectConstantBuffer(ID3D11Device* device, ID3D11DeviceContext* context, UINT maxDisplayBytes)
{
    LOG("---------Inspect Vetext Shader Constant-------------");
    for (const auto& cb : ConstantBuffersVS)
    {
        if (!cb.bIsValid)
            return false;
        InspectBufferContent(static_cast<ID3D11Buffer*>(cb.Resource), device, context, maxDisplayBytes);
    }
    LOG("---------Inspect Pixel Shader Constant-------------");
    for (const auto& cb : ConstantBuffersPS)
    {
        if (!cb.bIsValid)
            return false;
        InspectBufferContent(static_cast<ID3D11Buffer*>(cb.Resource), device, context, maxDisplayBytes);
    }

    return !ConstantBuffersVS.empty() || !ConstantBuffersPS.empty();
}

bool FD3DContextDebugger::InspectVertexBuffer(ID3D11Device* device, ID3D11DeviceContext* context, UINT maxDisplayBytes)
{
    LOG("---------Inspect Vertex Buffer Constant-------------");
    for (const auto& vb : VertexBuffers)
    {
        if (!vb.bIsValid) 
            return false;
        InspectBufferContent(static_cast<ID3D11Buffer*>(vb.Resource), device, context, maxDisplayBytes);
    }
    return !VertexBuffers.empty();
}

bool FD3DContextDebugger::ValidateResource(const FResourceBinding& Resource) const
{
    return Resource.bIsValid && Resource.Resource != nullptr;
}

bool FD3DContextDebugger::InspectBufferContent(ID3D11Buffer* buffer, ID3D11Device* device, ID3D11DeviceContext* context, UINT maxDisplayBytes)
{
    if (!buffer || !device || !context) {
        LOG("Invalid buffer or device or context");
        return false;
    }

    D3D11_BUFFER_DESC desc;
    buffer->GetDesc(&desc);

    LOG("Buffer desc - ByteWidth: %u, Usage: %d, BindFlags: %d, CPUAccessFlags: %d",
        desc.ByteWidth, desc.Usage, desc.BindFlags, desc.CPUAccessFlags);

    // 스테이징 버퍼 생성
    D3D11_BUFFER_DESC stagingDesc = desc;
    stagingDesc.Usage = D3D11_USAGE_STAGING;
    stagingDesc.BindFlags = 0;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    stagingDesc.MiscFlags = 0;

    ID3D11Buffer* stagingBuffer = nullptr;
    HRESULT hr = device->CreateBuffer(&stagingDesc, nullptr, &stagingBuffer);
    if (FAILED(hr)) {
        LOG("Failed to create staging buffer, hr = 0x%08X", hr);
        return false;
    }

    // 데이터 복사
    context->CopyResource(stagingBuffer, buffer);

    // 데이터 매핑 및 출력
    D3D11_MAPPED_SUBRESOURCE mapped;
    hr = context->Map(stagingBuffer, 0, D3D11_MAP_READ, 0, &mapped);
    if (SUCCEEDED(hr)) {
        LOG("Buffer content at %p, first %u bytes:", mapped.pData, std::min(maxDisplayBytes, desc.ByteWidth));

        const unsigned char* data = static_cast<const unsigned char*>(mapped.pData);
        
        // 16바이트씩 한 줄에 출력하기 위한 로깅
        std::string byteString = "";
        for (UINT i = 0; i < std::min(maxDisplayBytes, desc.ByteWidth); i++) {
            if (i % 16 == 0) {
                // 첫 번째 줄이 아니면 이전 줄 출력
                if (i > 0) {
                    LOG("%s", byteString.c_str());
                    byteString = "";
                }
                char indexStr[10];
                sprintf_s(indexStr, "\n%04X: ", i);
                byteString += indexStr;
            }

            // 바이트 값을 16진수로 포맷팅하여 추가
            char hexByte[4];
            sprintf_s(hexByte, "%02X ", data[i]);
            byteString += hexByte;
        }

        // 마지막 줄 출력
        if (!byteString.empty()) {
            LOG("%s", byteString.c_str());
        }
        LOG("\n");

        // float 형태로 출력 (4바이트씩 해석)
        LOG("As floats:");
        for (UINT i = 0; i < std::min(maxDisplayBytes, desc.ByteWidth); i += 4) {
            if (i % 16 == 0) {
                LOG("\n%04X: ", i);
            }

            // 남은 바이트가 4보다 작으면 처리하지 않음
            if (i + 3 < std::min(maxDisplayBytes, desc.ByteWidth)) {
                float value = *reinterpret_cast<const float*>(&data[i]);
                LOG("%.3f ", value);
            }
        }
        LOG("\n");

        // 데이터가 모두 0인지 확인
        bool allZero = true;
        for (UINT i = 0; i < desc.ByteWidth && allZero; i++) {
            if (data[i] != 0) {
                allZero = false;
            }
        }

        if (allZero) {
            LOG("WARNING: Buffer content is all zeros!");
        }


        context->Unmap(stagingBuffer, 0);
    }
    else {
        LOG("Failed to map staging buffer, hr = 0x%08X", hr);
    }

    stagingBuffer->Release();

    return true;
}

void FD3DContextDebugger::EnableD3D11DebugMessages(ID3D11Device* device)
{
    ID3D11Debug* debugDevice = nullptr;
    if (SUCCEEDED(device->QueryInterface(__uuidof(ID3D11Debug), (void**)&debugDevice)))
    {
        ID3D11InfoQueue* infoQueue = nullptr;
        if (SUCCEEDED(debugDevice->QueryInterface(__uuidof(ID3D11InfoQueue), (void**)&infoQueue)))
        {
            // 모든 메시지 심각도 기록
            infoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, TRUE);
            infoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, TRUE);
            infoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_WARNING, TRUE);

            // 축적된 모든 메시지 출력
            UINT64 messageCount = infoQueue->GetNumStoredMessages();
            for (UINT64 i = 0; i < messageCount; i++)
            {
                SIZE_T messageSize = 0;
                infoQueue->GetMessage(i, nullptr, &messageSize);

                D3D11_MESSAGE* message = (D3D11_MESSAGE*)malloc(messageSize);
                if (message)
                {
                    infoQueue->GetMessage(i, message, &messageSize);
                    LOG("D3D11 Message %llu: %s (ID=%d, Severity=%d)",
                        i, message->pDescription, message->ID, message->Severity);
                    free(message);
                }
            }

            infoQueue->Release();
        }
        debugDevice->Release();
    }
}