#pragma once
#include "D3DShader.h"

class UPixelShader : public UShaderBase
{
public:
    UPixelShader() = default;
    virtual ~UPixelShader();

    // 쉐이더 특화 기능
    ID3D11PixelShader* GetShader() const { return PixelShader; }

protected:

    // IResource 인터페이스 구현
    bool LoadImpl(IRenderHardware* RenderHardware, const std::wstring& Path) override;
    bool LoadAsyncImpl(IRenderHardware* RenderHardware, const std::wstring& Path) override;
    void ReleaseImpl() override { ReleasePixel(); }

private:
    void CalculateMemoryUsage() override;
    void ReleasePixel();
private:
    ID3D11PixelShader* PixelShader = nullptr;
};