#pragma once
#pragma comment(lib, "user32")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")
#pragma comment(lib, "dxguid")

#include <d3d11.h>
#include <d3dcompiler.h>
#include <directxmath.h>

#include <vector>
#include <string>

// 1. 상수 버퍼 레이아웃 관리
class FShaderParameterLayout
{
public:
    struct ParameterInfo {
        std::string Name;
        size_t Offset;
        size_t Size;
    };

    std::vector<ParameterInfo> Parameters;
    size_t BufferSize;

    D3D11_SHADER_DESC ShaderDesc;

    // 쉐이더 리플렉션으로부터 레이아웃 구성
    void InitFromReflection(ID3D11ShaderReflectionConstantBuffer* CBReflection)
    {
        D3D11_SHADER_BUFFER_DESC BufferDesc;
        CBReflection->GetDesc(&BufferDesc);

        Name = BufferDesc.Name

    }

};

// 2. 상수 버퍼 관리자
class FConstantBufferManager
{
private:
    struct BufferInfo {
        ID3D11Buffer* Buffer;
        FShaderParameterLayout Layout;
        void* StagingMemory;  // CPU 측 스테이징 메모리
    };

    std::unordered_map<std::string, BufferInfo> ConstantBuffers;
    ID3D11Device* Device;

public:
    FConstantBufferManager(ID3D11Device* InDevice) : Device(InDevice) {}

    // 쉐이더 리플렉션에서 상수 버퍼 등록
    void RegisterBuffersFromShader(ID3D11ShaderReflection* Reflection);

    // 특정 버퍼의 데이터 업데이트
    template<typename T>
    void UpdateBufferData(const std::string& BufferName, const T& Data);

    // 버퍼와 슬롯 바인딩 정보 반환
    std::vector<std::pair<ID3D11Buffer*, UINT>> GetBufferBindings(const std::string& ShaderName);
};