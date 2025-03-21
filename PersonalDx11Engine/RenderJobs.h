// RenderJob.h
#pragma once
#include "RenderStateInterface.h"
#include "RenderJobInterface.h"
#include "Math.h"
#include <d3d11.h>

// ���� ����
class FRenderContext;

// �޽� ������ �۾�
class FMeshRenderJob : public IRenderJob
{
public:
    FMeshRenderJob(ERenderStateType InStateType = ERenderStateType::Solid)
        : StateType(InStateType) {
    }

    void Execute(FRenderContext& Context) override
	{
        // ���� ���ؽ�Ʈ�� �޼��带 Ȱ���Ͽ� ������ ����

        // 1. ���� ���ε�
		Context.BindVertexBuffer(VertexBuffer, Stride, Offset);
		if (IndexBuffer)
		{
			Context.BindIndexBuffer(IndexBuffer);
		}

		// 2. ���̴� ���ε�
		Context.BindShader(VS, PS, InputLayout);

		// 3. ��� ���� ������Ʈ �� ���ε�
		for (const auto& CB : VSConstantBuffers)
		{
			Context.BindConstantBuffer(CB.Slot, CB.Buffer, CB.Data.data(), CB.Data.size(), true);
		}

		for (const auto& CB : PSConstantBuffers)
		{
			Context.BindConstantBuffer(CB.Slot, CB.Buffer, CB.Data.data(), CB.Data.size(), false);
		}

		// 4. �ؽ�ó �� ���÷� ���ε�
		for (const auto& Tex : Textures)
		{
			Context.BindShaderResource(Tex.Slot, Tex.SRV);
		}

		for (const auto& Samp : Samplers)
		{
			Context.BindSamplerState(Samp.Slot, Samp.Sampler);
		}

		// 5. ��ο� �� ����
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

    // ������ ���� �޼����
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