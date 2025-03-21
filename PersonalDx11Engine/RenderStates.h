#pragma once
#include "RenderStateInterface.h"

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

    void SetWireFrameRSState(ID3D11RasterizerState* state)
    {
        WireframeRasterizerState = state;
        if (WireframeRasterizerState)
        {
            WireframeRasterizerState->AddRef();
        }
    }
};

class FSolidState : public IRenderState
{
private:
    ID3D11RasterizerState* SolidRasterizerState;
    ID3D11RasterizerState* PreviousRasterizerState;

public:
    ~FSolidState()
    {
        if (SolidRasterizerState)
        {
            SolidRasterizerState->Release();
        }
        if (PreviousRasterizerState)
        {
            PreviousRasterizerState->Release();
        }
    }

    void Apply(ID3D11DeviceContext* Context) override
    {
        Context->RSGetState(&PreviousRasterizerState);
        Context->RSSetState(SolidRasterizerState);
    }
    void Restore(ID3D11DeviceContext* Context) override
    {
        Context->RSSetState(PreviousRasterizerState);
    }

    ERenderStateType GetType() const override
    {
        return ERenderStateType::Solid;
    }

    void SetSolidRSS(ID3D11RasterizerState* state)
    {
        SolidRasterizerState = state;
        if (SolidRasterizerState)
        {
            SolidRasterizerState->AddRef();
        }
    }
};