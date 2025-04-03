#pragma once
#include "RenderDefines.h"
#include "RenderStateInterface.h"
#include <memory>

class FWireframeState : public IRenderState
{
private:
    ID3D11RasterizerState* WireframeRasterizerState;
    ID3D11RasterizerState* PreviousRasterizerState;
public:
    ~FWireframeState()
    {
        if (WireframeRasterizerState)
        {
            WireframeRasterizerState->Release();
        }
        if (PreviousRasterizerState)
        {
            PreviousRasterizerState->Release();
        }
    }

    void Apply(ID3D11DeviceContext* Context) override
    {
        Context->RSGetState(&PreviousRasterizerState);
        Context->RSSetState(WireframeRasterizerState);
    }

    void Restore(ID3D11DeviceContext* Context) override
    {
        Context->RSSetState(PreviousRasterizerState);
    }

    ERenderStateType GetType() const override
    {
        return ERenderStateType::Wireframe;
    }

    void SetWireframeRasterizerState(ID3D11RasterizerState* state)
    {
        WireframeRasterizerState = state;
        if (WireframeRasterizerState)
        {
            WireframeRasterizerState->AddRef();
        }
    }

    static std::unique_ptr<FWireframeState> Create(ID3D11Device* Device);

};

class FSolidState : public IRenderState
{
private:
    ID3D11RasterizerState* SolidRasterizerState;
    ID3D11RasterizerState* PreviousRasterizerState;

    ID3D11SamplerState* SolidSamplerState;
    ID3D11SamplerState* PreviousSamplerState;

public:
    ~FSolidState();

    void Apply(ID3D11DeviceContext* Context) override
    {
        Context->RSGetState(&PreviousRasterizerState);
        Context->RSSetState(SolidRasterizerState);

        Context->PSGetSamplers(0, 1, &PreviousSamplerState);
        Context->PSSetSamplers(0,1,&SolidSamplerState);
    }

    void Restore(ID3D11DeviceContext* Context) override
    {
        Context->RSSetState(PreviousRasterizerState);
        Context->PSSetSamplers(0, 1, &PreviousSamplerState);
    }

    ERenderStateType GetType() const override
    {
        return ERenderStateType::Solid;
    }

    void SetSolidRasterState(ID3D11RasterizerState* state)
    {
        SolidRasterizerState = state;
        if (SolidRasterizerState)
        {
            SolidRasterizerState->AddRef();
        }
    }

    void SetSolidSamplerState(ID3D11SamplerState* state)
    {
        SolidSamplerState = state;
        if (SolidSamplerState)
        {
            SolidSamplerState->AddRef();
        }
    }

    static std::unique_ptr<FSolidState> Create(ID3D11Device* Device);
};