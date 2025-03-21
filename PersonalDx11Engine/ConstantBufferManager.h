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

// 1. ��� ���� ���̾ƿ� ����
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

    // ���̴� ���÷������κ��� ���̾ƿ� ����
    void InitFromReflection(ID3D11ShaderReflectionConstantBuffer* CBReflection)
    {
        D3D11_SHADER_BUFFER_DESC BufferDesc;
        CBReflection->GetDesc(&BufferDesc);

        Name = BufferDesc.Name

    }

};

// 2. ��� ���� ������
class FConstantBufferManager
{
private:
    struct BufferInfo {
        ID3D11Buffer* Buffer;
        FShaderParameterLayout Layout;
        void* StagingMemory;  // CPU �� ������¡ �޸�
    };

    std::unordered_map<std::string, BufferInfo> ConstantBuffers;
    ID3D11Device* Device;

public:
    FConstantBufferManager(ID3D11Device* InDevice) : Device(InDevice) {}

    // ���̴� ���÷��ǿ��� ��� ���� ���
    void RegisterBuffersFromShader(ID3D11ShaderReflection* Reflection);

    // Ư�� ������ ������ ������Ʈ
    template<typename T>
    void UpdateBufferData(const std::string& BufferName, const T& Data);

    // ���ۿ� ���� ���ε� ���� ��ȯ
    std::vector<std::pair<ID3D11Buffer*, UINT>> GetBufferBindings(const std::string& ShaderName);
};