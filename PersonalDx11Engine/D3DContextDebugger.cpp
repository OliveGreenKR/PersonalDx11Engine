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
    if (vs) vs->Release();

    // 입력 레이아웃 캡처
    ID3D11InputLayout* layout = nullptr;
    DeviceContext->IAGetInputLayout(&layout);
    InputLayout.Type = "InputLayout";
    InputLayout.Name = "Main";
    InputLayout.Resource = layout;
    InputLayout.bIsValid = (layout != nullptr);
    if (layout) layout->Release();

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
    if (ps) ps->Release();

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
    if (dsv) dsv->Release();

    // 블렌드 상태 캡처
    ID3D11BlendState* blendState = nullptr;
    FLOAT blendFactor[4];
    UINT sampleMask;
    DeviceContext->OMGetBlendState(&blendState, blendFactor, &sampleMask);

    BlendState.Type = "BlendState";
    BlendState.Name = "Main";
    BlendState.Resource = blendState;
    BlendState.bIsValid = (blendState != nullptr);
    if (blendState) blendState->Release();

    // 5. 래스터라이저 스테이지
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
    if (depthStencilState) depthStencilState->Release();
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
            if (!hasValidRenderTarget) return false;
        }
    }
    return hasValidRenderTarget || DepthStencilView.bIsValid;
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

bool FD3DContextDebugger::ValidateResource(const FResourceBinding& Resource) const
{
    return Resource.bIsValid && Resource.Resource != nullptr;
}