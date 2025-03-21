// RenderJob.h
#pragma once
#include "RenderStateInterface.h"
#include "RenderJobInterface.h"
#include "Math.h"
#include <d3d11.h>

// 전방 선언
class FRenderContext;

// 메시 렌더링 작업
class FMeshRenderJob : public IRenderJob
{
public:
    FMeshRenderJob(ERenderStateType InStateType = ERenderStateType::Solid)
        : StateType(InStateType) {
    }

    void Execute(FRenderContext& Context) override
	{
        // 렌더 컨텍스트의 메서드를 활용하여 렌더링 수행

        // 1. 버퍼 바인딩
		Context.BindVertexBuffer(VertexBuffer, Stride, Offset);
		if (IndexBuffer)
		{
			Context.BindIndexBuffer(IndexBuffer);
		}

		// 2. 쉐이더 바인딩
		Context.BindShader(VS, PS, InputLayout);

		// 3. 상수 버퍼 업데이트 및 바인딩
		for (const auto& CB : VSConstantBuffers)
		{
			Context.BindConstantBuffer(CB.Slot, CB.Buffer, CB.Data.data(), CB.Data.size(), true);
		}

		for (const auto& CB : PSConstantBuffers)
		{
			Context.BindConstantBuffer(CB.Slot, CB.Buffer, CB.Data.data(), CB.Data.size(), false);
		}

		// 4. 텍스처 및 샘플러 바인딩
		for (const auto& Tex : Textures)
		{
			Context.BindShaderResource(Tex.Slot, Tex.SRV);
		}

		for (const auto& Samp : Samplers)
		{
			Context.BindSamplerState(Samp.Slot, Samp.Sampler);
		}

		// 5. 드로우 콜 실행
		if (IndexBuffer && IndexCount > 0)
		{
			Context.DrawIndexed(IndexCount);
		}
		else if (VertexCount > 0)
		{
			Context.Draw(VertexCount);
		}
	}

    ERenderStateType GetStateType() const override { return StateType; }

    // 데이터 설정 메서드들
    void SetVertexBuffer(ID3D11Buffer* InBuffer, UINT InStride, UINT InOffset) {
        VertexBuffer = InBuffer;
        Stride = InStride;
        Offset = InOffset;
    }

    void SetIndexBuffer(ID3D11Buffer* InBuffer, UINT InCount) {
        IndexBuffer = InBuffer;
        IndexCount = InCount;
    }

    void SetVertexCount(UINT InCount) {
        VertexCount = InCount;
    }

    void SetShaders(ID3D11VertexShader* InVS, ID3D11PixelShader* InPS, ID3D11InputLayout* InLayout) {
        VS = InVS;
        PS = InPS;
        InputLayout = InLayout;
    }

    struct ConstantBufferInfo {
        UINT Slot;
        ID3D11Buffer* Buffer;
        std::vector<uint8_t> Data;
    };

    void AddVSConstantBuffer(UINT InSlot, ID3D11Buffer* InBuffer, const void* InData, size_t InSize) {
        ConstantBufferInfo Info;
        Info.Slot = InSlot;
        Info.Buffer = InBuffer;
        Info.Data.resize(InSize);
        std::memcpy(Info.Data.data(), InData, InSize);
        VSConstantBuffers.push_back(Info);
    }

    void AddPSConstantBuffer(UINT InSlot, ID3D11Buffer* InBuffer, const void* InData, size_t InSize) {
        ConstantBufferInfo Info;
        Info.Slot = InSlot;
        Info.Buffer = InBuffer;
        Info.Data.resize(InSize);
        std::memcpy(Info.Data.data(), InData, InSize);
        PSConstantBuffers.push_back(Info);
    }

    void AddTexture(UINT InSlot, ID3D11ShaderResourceView* InSRV) {
        Textures.push_back({ InSlot, InSRV });
    }

    void AddSampler(UINT InSlot, ID3D11SamplerState* InSampler) {
        Samplers.push_back({ InSlot, InSampler });
    }

private:
    ERenderStateType StateType;

    ID3D11Buffer* VertexBuffer = nullptr;
    ID3D11Buffer* IndexBuffer = nullptr;
    UINT VertexCount = 0;
    UINT IndexCount = 0;
    UINT Stride = 0;
    UINT Offset = 0;

    ID3D11VertexShader* VS = nullptr;
    ID3D11PixelShader* PS = nullptr;
    ID3D11InputLayout* InputLayout = nullptr;

    struct TextureBinding {
        UINT Slot;
        ID3D11ShaderResourceView* SRV;
    };

    struct SamplerBinding {
        UINT Slot;
        ID3D11SamplerState* Sampler;
    };

    std::vector<ConstantBufferInfo> VSConstantBuffers;
    std::vector<ConstantBufferInfo> PSConstantBuffers;
    std::vector<TextureBinding> Textures;
    std::vector<SamplerBinding> Samplers;
};