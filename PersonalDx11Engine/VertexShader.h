#pragma once
#include "D3DShader.h"
#include "ResourceInterface.h"

class UVertexShader : public UShaderBase, public IResource
{
public:
    UVertexShader() = default;
    virtual ~UVertexShader() ;

    // IResource 인터페이스 구현
    bool Load(IRenderHardware* RenderHardware, const std::wstring& Path) override;
    bool LoadAsync(IRenderHardware* RenderHardware, const std::wstring& Path) override;
    bool IsLoaded() const override { return bIsLoaded; }
    void Release() override;
    size_t GetMemorySize() const override { return MemorySize; }
    EResourceType GetType() const override { return EResourceType::Shader; }

    // 쉐이더 특화 기능
    ID3D11VertexShader* GetShader() const { return VertexShader; }
private:
    void CalculateMemoryUsage() override;
private:
    ID3D11VertexShader* VertexShader = nullptr;
};