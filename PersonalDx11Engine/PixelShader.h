#pragma once
#include "D3DShader.h"
#include "ResourceInterface.h"

class UPixelShader : public UShaderBase, public IResource
{
public:
    UPixelShader() = default;
    ~UPixelShader() override;

    // IResource 인터페이스 구현
    bool Load(IRenderHardware* RenderHardware, const std::wstring& Path) override;
    bool LoadAsync(IRenderHardware* RenderHardware, const std::wstring& Path) override;
    bool IsLoaded() const override { return bIsLoaded; }
    void Release() override;
    size_t GetMemorySize() const override { return MemorySize; }
    EResourceType GetType() const override { return EResourceType::Shader; }

    // 쉐이더 특화 기능
    ID3D11PixelShader* GetShader() const { return PixelShader; }
private:
    void CalculateMemoryUsage() override;
private:
    ID3D11PixelShader* PixelShader = nullptr;
};