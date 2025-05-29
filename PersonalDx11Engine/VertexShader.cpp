#include "VertexShader.h"
#include "Debug.h"

UVertexShader::~UVertexShader()
{
    ReleaseVertex();
}

bool UVertexShader::LoadImpl(IRenderHardware* RenderHardware, const std::wstring& Path)
{
    if (!RenderHardware || !RenderHardware->IsDeviceReady())
        return false;

    // 이미 로드된 경우 재사용
    if (IsLoaded())
        return true;

    // 이전 리소스 해제
    Release();

    // 쉐이더 컴파일
    ID3DBlob* VSBlob = nullptr;
    HRESULT hr = CompileShader(Path.c_str(), "mainVS", "vs_5_0", &VSBlob);

    if (FAILED(hr) || !VSBlob)
    {
        LOG("Failed to compile vertex shader: %S", Path.c_str());
        return false;
    }

    // 쉐이더 객체 생성
    auto Device = RenderHardware->GetDevice();
    auto bufferPtr = VSBlob->GetBufferPointer();
    auto bufferSize = VSBlob->GetBufferSize();

    hr = RenderHardware->GetDevice()->CreateVertexShader(
        VSBlob->GetBufferPointer(),
        VSBlob->GetBufferSize(),
        nullptr,
        &VertexShader);

    if (FAILED(hr))
    {
        LOG("Failed to create vertex shader: %S", Path.c_str());
        VSBlob->Release();
        return false;
    }

    // 쉐이더 메타데이터 채우기
    if (!FillShaderMeta(RenderHardware->GetDevice(), VSBlob))
    {
        VSBlob->Release();
        Release();
        return false;
    }

    // 메모리 사용량 계산
    CalculateMemoryUsage();

    return true;
}

bool UVertexShader::LoadAsyncImpl(IRenderHardware* RenderHardware, const std::wstring& Path)
{
    // 비동기 로드 구현 (단순화를 위해 여기서는 동기 로드로 대체)
    LOG("Async loading not implemented, falling back to sync loading");
    return LoadImpl(RenderHardware, Path);
}

void UVertexShader::ReleaseVertex()
{
    if (VertexShader)
    {
        VertexShader->Release();
        VertexShader = nullptr;
    }
}

void UVertexShader::CalculateMemoryUsage()
{
    UShaderBase::CalculateMemoryUsage();
    MemorySize += (VertexShader ? 128 : 0);
}
