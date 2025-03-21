// RenderJob.h
#pragma once
#include "RenderStateInterface.h"
#include "Math.h"
#include <d3d11.h>

// ���� ����
class FRenderContext;


class FRenderJobBase
{
public:
    virtual ~FRenderJobBase() = default;
    virtual void Execute(FRenderContext* Context) {};
    virtual ERenderStateType GetStateType() const { return ERenderStateType::None; }
};


// �޽�-�ؽ�ó ������ �۾�
class FMeshRenderJob : public FRenderJobBase
{
public:
    FMeshRenderJob(ERenderStateType InStateType = ERenderStateType::Solid)
        : StateType(InStateType) {
    }

    void Execute(FRenderContext* Context) override
	{
        // ���� ���ؽ�Ʈ�� �޼��带 Ȱ���Ͽ� ������ ����

        // 1. ���� ���ε�
        Context->BindVertexBuffer(VertexBuffer, Stride, Offset);
		if (IndexBuffer)
		{
            Context->BindIndexBuffer(IndexBuffer);
		}

		// 2. ���̴� ���ε�
        Context->BindShader(VS, PS, InputLayout);

		// 3. ��� ���� ������Ʈ �� ���ε�
		for (const auto& CB : VSConstantBuffers)
		{
            Context->BindConstantBuffer(CB.Slot, CB.Buffer, CB.Data.data(), CB.Data.size(), true);
		}

		for (const auto& CB : PSConstantBuffers)
		{
            Context->BindConstantBuffer(CB.Slot, CB.Buffer, CB.Data.data(), CB.Data.size(), false);
		}

		// 4. �ؽ�ó �� ���÷� ���ε�
		for (const auto& Tex : Textures)
		{
            Context->BindShaderResource(Tex.Slot, Tex.SRV);
		}

		for (const auto& Samp : Samplers)
		{
            Context->BindSamplerState(Samp.Slot, Samp.Sampler);
		}

		// 5. ��ο� �� ����
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