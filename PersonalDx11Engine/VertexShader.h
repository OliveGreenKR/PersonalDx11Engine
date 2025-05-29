#pragma once
#include "D3DShader.h"

class UVertexShader : public UShaderBase
{
public:
    UVertexShader() = default;
    virtual ~UVertexShader() ;

    // 쉐이더 특화 기능
    ID3D11VertexShader* GetShader() const { return VertexShader; }

protected:
    bool LoadImpl(IRenderHardware* RenderHardware, const std::wstring& Path) override;
    bool LoadAsyncImpl(IRenderHardware* RenderHardware, const std::wstring& Path) override;
    void ReleaseImpl() override { ReleaseVertex(); }

private:
    void CalculateMemoryUsage() override;
    void ReleaseVertex();
private:
    ID3D11VertexShader* VertexShader = nullptr;
};