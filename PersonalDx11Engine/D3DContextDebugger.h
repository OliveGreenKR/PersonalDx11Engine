#pragma once
#include <d3d11.h>
#include <string>
#include <vector>
#include <memory>

class FD3DContextDebugger
{
public:
    struct FResourceBinding
    {
        std::string Type;
        std::string Name;
        UINT Slot;
        void* Resource;
        bool bIsValid;
    };

    FD3DContextDebugger() = default;
    ~FD3DContextDebugger();

    // 컨텍스트의 모든 바인딩 리소스 캡처
    void CaptureBindings(ID3D11DeviceContext* DeviceContext);
    void EnableD3D11DebugMessages(ID3D11Device* device);

    // 캡처된 리소스의 유효성 검사
    bool ValidateAllBindings() const;

    // 디버그 출력 
    void PrintBindings() const;

    // 특정 유형의 리소스에 대한 유효성 검사
    bool ValidateVertexBuffers() const;
    bool ValidateIndexBuffers() const;
    bool ValidateShaders() const;
    bool ValidateConstantBuffers() const;
    bool ValidateShaderResources() const;
    bool ValidateSamplers() const;
    bool ValidateRenderTargets() const;
    bool ValidateRasterizerState() const;
    bool ValidateBlendState() const;
    bool ValidateDepthStencilState() const;

    bool InspectConstantBuffer(ID3D11Device* device, ID3D11DeviceContext* context , UINT maxDisplayBytes = 64);
    bool InspectVertexBuffer(ID3D11Device* device, ID3D11DeviceContext* context, UINT maxDisplayBytes = 64);

    template<typename T>
    std::vector<T> GetBufferData(ID3D11Buffer* buffer, ID3D11Device* device, ID3D11DeviceContext* context)
    {
        std::vector<T> result;

        if (!buffer || !context)
            return result; // 유효성 체크: nullptr이면 빈 벡터 반환

        // 1. 버퍼 설명 가져오기
        D3D11_BUFFER_DESC desc;
        buffer->GetDesc(&desc);

        // 버퍼 크기에서 요소 개수 계산
        UINT elementCount = desc.ByteWidth / sizeof(T);
        if (desc.ByteWidth % sizeof(T) != 0)
            return result; // 크기가 T의 배수가 아니면 오류 방지

        // 2. 스테이징 버퍼 생성 (CPU 읽기 가능)
        D3D11_BUFFER_DESC stagingDesc = desc;
        stagingDesc.Usage = D3D11_USAGE_STAGING;
        stagingDesc.BindFlags = 0; // 바인딩 없음
        stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

        ID3D11Buffer* stagingBuffer = nullptr;
        HRESULT hr = device->CreateBuffer(&stagingDesc, nullptr, &stagingBuffer);
        if (FAILED(hr))
            return result; // 생성 실패 시 빈 벡터 반환

        // 3. 원본 버퍼 데이터를 스테이징 버퍼로 복사
        context->CopyResource(stagingBuffer, buffer);

        // 4. 스테이징 버퍼를 매핑하여 데이터 읽기
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        hr = context->Map(stagingBuffer, 0, D3D11_MAP_READ, 0, &mappedResource);
        if (SUCCEEDED(hr))
        {
            // 데이터 크기에 맞게 벡터 크기 예약
            result.resize(elementCount);

            // 매핑된 메모리에서 데이터 복사
            memcpy(result.data(), mappedResource.pData, desc.ByteWidth);

            // 매핑 해제
            context->Unmap(stagingBuffer, 0);
        }

        // 5. 스테이징 버퍼 해제
        stagingBuffer->Release();

        return result;
    }

private:

    //참조한 포인터 null처리
    void Release();
    // 버퍼 내용 검사
    bool InspectBufferContent(ID3D11Buffer* buffer, ID3D11Device* device, ID3D11DeviceContext* context , UINT maxDisplayBytes = 64);

    // 캡처된 리소스 저장
    std::vector<FResourceBinding> VertexBuffers;
    std::vector<FResourceBinding> IndexBuffers;
    std::vector<FResourceBinding> ConstantBuffersVS;
    std::vector<FResourceBinding> ConstantBuffersPS;
    std::vector<FResourceBinding> ShaderResources;
    std::vector<FResourceBinding> Samplers;
    FResourceBinding RasterizerState;
    FResourceBinding BlendState;
    FResourceBinding DepthStencilState;
    FResourceBinding RenderTargets[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT];
    FResourceBinding DepthStencilView;
    FResourceBinding VertexShader;
    FResourceBinding PixelShader;
    FResourceBinding InputLayout;

    // 리소스 유효성 검사 (nullptr 체크 및 추가 검증)
    bool ValidateResource(const FResourceBinding& Resource) const;

};