#pragma once
#include "ResourceInterface.h"
#include "RenderHardwareInterface.h"
#include <string>
#include <memory>
#include <thread>
#include <vector>

enum class ETextureFormat
{
    RGBA8,          // 32비트 RGBA 포맷 (8비트/채널)
    BGRA8,          // 32비트 BGRA 포맷 (8비트/채널)
    DXT1,           // DXT1 압축 포맷
    DXT5,           // DXT5 압축 포맷
    R8,             // 8비트 단일 채널
    RG8,            // 16비트 이중 채널 (8비트/채널)
    RGBA16F,        // 64비트 부동소수점 RGBA (16비트/채널)
    RGBA32F,        // 128비트 부동소수점 RGBA (32비트/채널)
    DEPTH24_STENCIL8 // 24비트 깊이 + 8비트 스텐실
};

struct FTextureDesc
{
    uint32_t Width = 0;
    uint32_t Height = 0;
    uint32_t MipLevels = 1;
    ETextureFormat Format = ETextureFormat::RGBA8;
    bool bDynamic = false;           // CPU 쓰기 가능 여부
    bool bRenderTarget = false;      // 렌더 타겟으로 사용할지 여부
    bool bShaderResource = true;     // 셰이더 리소스로 사용할지 여부
    bool bUnorderedAccess = false;   // UAV로 사용할지 여부   

    FTextureDesc() = default;
    FTextureDesc(uint32_t InWidth, uint32_t InHeight, ETextureFormat InFormat = ETextureFormat::RGBA8)
        : Width(InWidth), Height(InHeight), Format(InFormat) {
    }
};

class UTexture2D : public IResource
{
public:
    UTexture2D();
    virtual ~UTexture2D() override;

    // IResource 인터페이스 구현
    virtual bool IsLoaded() const override { return bIsLoaded; }
    virtual void Release() override;
    virtual size_t GetMemorySize() const override { return MemorySize; }
    virtual const std::wstring& GetPath() const override { return RscPath; }
    bool Load(IRenderHardware* RenderHardware, const std::wstring& FilePath) override;
    bool LoadAsync(IRenderHardware* RenderHardware, const std::wstring& FilePath) override;
    EResourceType GetType() const override { return EResourceType::Texture; }

    //텍스처 메소드
    bool CreateEmpty(IRenderHardware* RenderHardware, const FTextureDesc& Desc, const void* InitialData = nullptr);

    // 데이터 업데이트 메서드
    bool UpdateData(ID3D11DeviceContext* DeviceContext, const void* Data, uint32_t Pitch = 0);

    // 텍스처 정보 조회
    uint32_t GetWidth() const { return TextureDesc.Width; }
    uint32_t GetHeight() const { return TextureDesc.Height; }
    ETextureFormat GetFormat() const { return TextureDesc.Format; }
    const FTextureDesc& GetTextureDesc() const { return TextureDesc; }

    // 리소스 접근자
    ID3D11Texture2D* GetTexture() const { return Texture; }
    ID3D11ShaderResourceView* GetShaderResourceView() const { return ShaderResourceView; }
    ID3D11RenderTargetView* GetRenderTargetView() const { return RenderTargetView; }
    ID3D11UnorderedAccessView* GetUnorderedAccessView() const { return UnorderedAccessView; }

    // 렌더 타겟 관련 메서드
    void Clear(ID3D11DeviceContext* DeviceContext, const float Color[4]);

    // 비동기 로딩 상태 확인
    bool IsLoadingComplete() const { return !bIsLoading || bIsLoaded; }
    bool IsLoading() const { return bIsLoading; }

protected:
    virtual bool LoadImpl(IRenderHardware* RenderHardware, const std::wstring& FilePath);
    virtual bool LoadAsyncImpl(IRenderHardware* RenderHardware, const std::wstring& FilePath);
    virtual void ReleaseImpl() {};
private:
    // DXGI 포맷 변환 헬퍼
    DXGI_FORMAT GetDXGIFormat(ETextureFormat Format) const;

    // 메모리 사용량 계산
    void CalculateMemoryUsage();

    // 비동기 로딩 완료 콜백
    void OnLoadingComplete(bool bSuccess);

    void ReleaseTextureBase();

private:
    // 텍스처 리소스
    ID3D11Texture2D* Texture = nullptr;
    ID3D11ShaderResourceView* ShaderResourceView = nullptr;
    ID3D11RenderTargetView* RenderTargetView = nullptr;
    ID3D11UnorderedAccessView* UnorderedAccessView = nullptr;

    // 텍스처 정보
    FTextureDesc TextureDesc;
    std::wstring FilePath;

    // 상태 플래그
    bool bIsLoaded = false;
    bool bIsLoading = false;
    std::wstring RscPath = L"NONE";

    // 메모리 관리
    size_t MemorySize = 0;

    // 비동기 로딩 컨텍스트 구조체
    struct FAsyncLoadingContext
    {
        std::wstring FilePath;
        IRenderHardware* RenderHardware;
        std::vector<BYTE> PixelData;
        UINT Width = 0;
        UINT Height = 0;
        bool bComplete = false;
        bool bSuccess = false;
        std::thread LoadingThread;

        FAsyncLoadingContext(IRenderHardware* InRenderHardware, const std::wstring& InFilePath)
            : RenderHardware(InRenderHardware), FilePath(InFilePath)
        {
        }

        ~FAsyncLoadingContext()
        {
            if (LoadingThread.joinable())
            {
                LoadingThread.join();
            }
        }
    };
    //비동기 컨텍스트
    std::unique_ptr<FAsyncLoadingContext> AsyncLoadingContext;
};