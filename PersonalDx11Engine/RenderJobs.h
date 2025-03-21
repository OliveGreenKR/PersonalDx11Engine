// RenderJob.h
#pragma once
#include "RenderStateInterface.h"
#include "Math.h"
#include <d3d11.h>

// 전방 선언
class FRenderContext;


class FRenderJobBase
{
public:
    virtual ~FRenderJobBase() = default;
    virtual void Execute(FRenderContext* Context) {};
    virtual ERenderStateType GetStateType() const { return ERenderStateType::None; }
};


// 메시-텍스처 렌더링 작업
class FMeshRenderJob : public FRenderJobBase
{
public:
    FMeshRenderJob(ERenderStateType InStateType = ERenderStateType::Solid)
        : StateType(InStateType) {
    }

    void Execute(FRenderContext* Context) override
	{
        // 렌더 컨텍스트의 메서드를 활용하여 렌더링 수행

        // 1. 버퍼 바인딩
        Context->BindVertexBuffer(VertexBuffer, Stride, Offset);
		if (IndexBuffer)
		{
            Context->BindIndexBuffer(IndexBuffer);
		}

		// 2. 쉐이더 바인딩
        Context->BindShader(VS, PS, InputLayout);

		// 3. 상수 버퍼 업데이트 및 바인딩
		for (const auto& CB : VSConstantBuffers)
		{
            Context->BindConstantBuffer(CB.Slot, CB.Buffer, CB.Data.data(), CB.Data.size(), true);
		}

		for (const auto& CB : PSConstantBuffers)
		{
            Context->BindConstantBuffer(CB.Slot, CB.Buffer, CB.Data.data(), CB.Data.size(), false);
		}

		// 4. 텍스처 및 샘플러 바인딩
		for (const auto& Tex : Textures)
		{
            Context->BindShaderResource(Tex.Slot, Tex.SRV);
		}

		for (const auto& Samp : Samplers)
		{
            Context->BindSamplerState(Samp.Slot, Samp.Sampler);
		}

		// 5. 드로우 콜 실행
		if (IndexBuffer && IndexCount > 0)
		{
            Context->DrawIndexed(IndexCount);
		}
		else if (VertexCount > 0)
		{
            Context->Draw(VertexCount);
		}
	}

    ERenderStateType GetStateType() const override { return StateType; }

    struct ConstantBufferInfo {
        UINT Slot;
        ID3D11Buffer* Buffer;
        std::vector<uint8_t> Data;
    };

    struct TextureBinding {
        UINT Slot;
        ID3D11ShaderResourceView* SRV;
    };

    struct SamplerBinding {
        UINT Slot;
        ID3D11SamplerState* Sampler;
    };


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


    std::vector<ConstantBufferInfo> VSConstantBuffers;
    std::vector<ConstantBufferInfo> PSConstantBuffers;
    std::vector<TextureBinding> Textures;
    std::vector<SamplerBinding> Samplers;
};