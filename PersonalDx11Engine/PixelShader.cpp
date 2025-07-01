#include "PixelShader.h"
#include "Debug.h"

UPixelShader::~UPixelShader()
{
    ReleasePixel();
}

bool UPixelShader::LoadImpl(IRenderHardware* RenderHardware, const std::wstring& Path)
{
    if (!RenderHardware || !RenderHardware->IsDeviceReady())
        return false;

    // 이미 로드된 경우 재사용
    if (IsLoaded())
        return true;

    // 이전 리소스 해제
    Release();

    // 쉐이더 컴파일
    ID3DBlob* PSBlob = nullptr;
    HRESULT hr = CompileShader(Path.c_str(), "mainPS", "ps_5_0", &PSBlob);

    if (FAILED(hr) || !PSBlob)
    {
        LOG_WARNING("Failed to compile pixel shader: %S", Path.c_str());
        return false;
    }

    // 쉐이더 객체 생성
    hr = RenderHardware->GetDevice()->CreatePixelShader(
        PSBlob->GetBufferPointer(),
        PSBlob->GetBufferSize(),
        nullptr,
        &PixelShader);

    if (FAILED(hr))
    {
        LOG_WARNING("Failed to create vertex shader: %S", Path.c_str());
        PSBlob->Release();
        return false;
    }

    // 쉐이더 메타데이터 채우기
    if (!FillShaderMeta(RenderHardware->GetDevice(), PSBlob))
    {
        PSBlob->Release();
        Release();
        return false;
    }

    // 메모리 사용량 계산
    CalculateMemoryUsage();

    return true;
}

bool UPixelShader::LoadAsyncImpl(IRenderHardware* RenderHardware, const std::wstring& Path)
{
    // 비동기 로드 구현 (단순화를 위해 여기서는 동기 로드로 대체)
    LOG("Async loading not implemented, falling back to sync loading");
    return LoadImpl(RenderHardware, Path);
}

void UPixelShader::CalculateMemoryUsage()
{
    UShaderBase::CalculateMemoryUsage();
    MemorySize += (PixelShader ? 128 : 0);
}

void UPixelShader::ReleasePixel()
{
    if (PixelShader)
    {
        PixelShader->Release();
        PixelShader = nullptr;
    }
}
