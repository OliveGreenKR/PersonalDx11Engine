#pragma once
#include "RenderHardwareInterface.h"
#include <vector>
#include <string>
#include <type_traits>

class UShaderBase
{
protected:
    // 상수 버퍼 변수 정보
    struct FInternalConstantBufferVariable
    {
        std::string Name;
        UINT Offset;
        UINT Size;
        D3D_SHADER_VARIABLE_CLASS Type;
        UINT Elements;
        UINT Rows;
        UINT Columns;
    };
    // 상수 버퍼 
    struct FInternalConstantBufferInfo
    {
        std::string Name;
        UINT Size;
        UINT BindPoint;
        ID3D11Buffer* Buffer = nullptr;
        std::vector<FInternalConstantBufferVariable> Variables; 
    };
    // 리소스 바인딩 정보
    struct FInternalResourceBindInfo
    {
        std::string Name;
        D3D_SHADER_INPUT_TYPE Type;
        UINT BindPoint;
        UINT BindCount;
    };

protected:
    ID3D11InputLayout* InputLayout = nullptr;
    std::vector<FInternalConstantBufferInfo> ConstantBuffers;
    std::vector<FInternalResourceBindInfo> ResourceBindingMeta;
    bool bIsLoaded = false;
    size_t MemorySize = 0;

public:
    UShaderBase() = default;
    virtual ~UShaderBase();

    const std::vector<FInternalConstantBufferInfo>& GetAllConstantBufferInfo() { return ConstantBuffers; }
    const std::vector<FInternalResourceBindInfo>& GetAllResourceBindInfo() { return ResourceBindingMeta; }

    ID3D11InputLayout* GetInputLayout() const { return InputLayout; }
    ID3D11Buffer* GetConstantBuffer(uint32_t Slot) const;
    uint32_t GetConstantBufferSize(uint32_t Slot) const;
    const std::string& GetConstantBufferName(uint32_t Slot) const;

    // 메모리 관리 헬퍼
    virtual void CalculateMemoryUsage();
    virtual void Release();

protected:
    bool FillShaderMeta(ID3D11Device* Device, ID3DBlob* ShaderBlob);
    static HRESULT CompileShader(const wchar_t* filename, const char* entryPoint, const char* target, ID3DBlob** ppBlob);

private:
    bool CreateInputLayoutFromReflection(ID3D11ShaderReflection* Reflection,
                                         std::vector<D3D11_INPUT_ELEMENT_DESC>& OutLayout);
    void ExtractAndCreateConstantBuffers(ID3D11Device* Device,
                                         ID3D11ShaderReflection* Reflection,
                                         std::vector<FInternalConstantBufferInfo>& OutBuffers);
    void ExtractResourceBindings(ID3D11ShaderReflection* Reflection,
                                 std::vector<FInternalResourceBindInfo>& OutBindings);
};