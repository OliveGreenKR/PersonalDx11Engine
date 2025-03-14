#include "Texture.h"
#include <wincodec.h>
#include "Math.h"
UTexture2D::UTexture2D()
{
    // 초기화
}

UTexture2D::~UTexture2D()
{
    Release();
}

void UTexture2D::Release()
{
    // 비동기 로딩 컨텍스트 정리
    if (AsyncLoadingContext)
    {
        // 비동기 로딩 취소 로직이 필요하다면 여기서 구현
        AsyncLoadingContext.reset();
    }

    // Direct3D 리소스 해제
    if (UnorderedAccessView)
    {
        UnorderedAccessView->Release();
        UnorderedAccessView = nullptr;
    }

    if (RenderTargetView)
    {
        RenderTargetView->Release();
        RenderTargetView = nullptr;
    }

    if (ShaderResourceView)
    {
        ShaderResourceView->Release();
        ShaderResourceView = nullptr;
    }

    if (Texture)
    {
        Texture->Release();
        Texture = nullptr;
    }

    // 상태 초기화
    bIsLoaded = false;
    bIsLoading = false;
    MemorySize = 0;
}

bool UTexture2D::Load(IRenderHardware* RenderHardware, const std::wstring& InFilePath)
{
    if (!RenderHardware)
        return false;

    // 이미 로드된 경우 재사용
    if (bIsLoaded && InFilePath == FilePath)
        return true;

    // 이전 리소스 해제
    Release();

    // 파일 경로 저장
    FilePath = InFilePath;

    // WIC를 사용하여 이미지 로드
    IWICImagingFactory* WICFactory = nullptr;
    HRESULT hr = CoCreateInstance(
        CLSID_WICImagingFactory,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&WICFactory)
    );

    if (FAILED(hr))
        return false;

    // 디코더 생성
    IWICBitmapDecoder* Decoder = nullptr;
    hr = WICFactory->CreateDecoderFromFilename(
        FilePath.c_str(),
        nullptr,
        GENERIC_READ,
        WICDecodeMetadataCacheOnLoad,
        &Decoder
    );

    if (FAILED(hr))
    {
        WICFactory->Release();
        return false;
    }

    // 프레임 획득
    IWICBitmapFrameDecode* Frame = nullptr;
    hr = Decoder->GetFrame(0, &Frame);

    if (FAILED(hr))
    {
        Decoder->Release();
        WICFactory->Release();
        return false;
    }

    // BGRA 포맷으로 변환
    IWICFormatConverter* Converter = nullptr;
    hr = WICFactory->CreateFormatConverter(&Converter);

    if (FAILED(hr))
    {
        Frame->Release();
        Decoder->Release();
        WICFactory->Release();
        return false;
    }

    hr = Converter->Initialize(
        Frame,
        GUID_WICPixelFormat32bppBGRA,
        WICBitmapDitherTypeNone,
        nullptr,
        0.0f,
        WICBitmapPaletteTypeCustom
    );

    if (FAILED(hr))
    {
        Converter->Release();
        Frame->Release();
        Decoder->Release();
        WICFactory->Release();
        return false;
    }

    // 이미지 크기 획득
    UINT Width = 0, Height = 0;
    Converter->GetSize(&Width, &Height);

    // 픽셀 데이터를 위한 메모리 할당
    UINT RowPitch = Width * 4;
    UINT BufferSize = RowPitch * Height;
    std::vector<BYTE> PixelData(BufferSize);

    // 픽셀 데이터 복사
    hr = Converter->CopyPixels(nullptr, RowPitch, BufferSize, PixelData.data());

    if (FAILED(hr))
    {
        Converter->Release();
        Frame->Release();
        Decoder->Release();
        WICFactory->Release();
        return false;
    }

    // 텍스처 설명 설정
    TextureDesc.Width = Width;
    TextureDesc.Height = Height;
    TextureDesc.Format = ETextureFormat::BGRA8;

    // 빈 텍스처 생성 후 데이터 채움
    bool bCreateResult = CreateEmpty(RenderHardware, TextureDesc, PixelData.data());

    // WIC 리소스 정리
    Converter->Release();
    Frame->Release();
    Decoder->Release();
    WICFactory->Release();

    return bCreateResult;
}

bool UTexture2D::LoadAsync(IRenderHardware* RenderHardware, const std::wstring& InFilePath)
{
    if (!RenderHardware)
        return false;

    // 이미 로드된 경우 재사용
    if (bIsLoaded && InFilePath == FilePath)
        return true;

    // 이전 리소스 해제
    Release();

    // 파일 경로 저장
    FilePath = InFilePath;

    // 비동기 로딩 상태 설정
    bIsLoading = true;
    bIsLoaded = false;

    // 비동기 로딩 컨텍스트 생성
    AsyncLoadingContext = std::make_unique<FAsyncLoadingContext>(RenderHardware, InFilePath);

    // 비동기 로딩 스레드 시작
    AsyncLoadingContext->LoadingThread = std::thread([this]() {
        FAsyncLoadingContext* Context = AsyncLoadingContext.get();

        // COM 초기화 (스레드별 필요)
        CoInitializeEx(nullptr, COINIT_MULTITHREADED);

        // WIC를 사용하여 이미지 로드
        IWICImagingFactory* WICFactory = nullptr;
        HRESULT hr = CoCreateInstance(
            CLSID_WICImagingFactory,
            nullptr,
            CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&WICFactory)
        );

        if (FAILED(hr))
        {
            CoUninitialize();
            Context->bComplete = true;
            Context->bSuccess = false;
            return;
        }

        // 디코더 생성
        IWICBitmapDecoder* Decoder = nullptr;
        hr = WICFactory->CreateDecoderFromFilename(
            Context->FilePath.c_str(),
            nullptr,
            GENERIC_READ,
            WICDecodeMetadataCacheOnLoad,
            &Decoder
        );

        if (FAILED(hr))
        {
            WICFactory->Release();
            CoUninitialize();
            Context->bComplete = true;
            Context->bSuccess = false;
            return;
        }

        // 프레임 획득
        IWICBitmapFrameDecode* Frame = nullptr;
        hr = Decoder->GetFrame(0, &Frame);

        if (FAILED(hr))
        {
            Decoder->Release();
            WICFactory->Release();
            CoUninitialize();
            Context->bComplete = true;
            Context->bSuccess = false;
            return;
        }

        // BGRA 포맷으로 변환
        IWICFormatConverter* Converter = nullptr;
        hr = WICFactory->CreateFormatConverter(&Converter);

        if (FAILED(hr))
        {
            Frame->Release();
            Decoder->Release();
            WICFactory->Release();
            CoUninitialize();
            Context->bComplete = true;
            Context->bSuccess = false;
            return;
        }

        hr = Converter->Initialize(
            Frame,
            GUID_WICPixelFormat32bppBGRA,
            WICBitmapDitherTypeNone,
            nullptr,
            0.0f,
            WICBitmapPaletteTypeCustom
        );

        if (FAILED(hr))
        {
            Converter->Release();
            Frame->Release();
            Decoder->Release();
            WICFactory->Release();
            CoUninitialize();
            Context->bComplete = true;
            Context->bSuccess = false;
            return;
        }

        // 이미지 크기 획득
        Converter->GetSize(&Context->Width, &Context->Height);

        // 픽셀 데이터를 위한 메모리 할당
        UINT RowPitch = Context->Width * 4;
        UINT BufferSize = RowPitch * Context->Height;
        Context->PixelData.resize(BufferSize);

        // 픽셀 데이터 복사
        hr = Converter->CopyPixels(nullptr, RowPitch, BufferSize, Context->PixelData.data());

        // WIC 리소스 정리
        Converter->Release();
        Frame->Release();
        Decoder->Release();
        WICFactory->Release();
        CoUninitialize();

        // 결과 설정
        Context->bComplete = true;
        Context->bSuccess = SUCCEEDED(hr);
                                                     });

    return true;
}

void UTexture2D::OnLoadingComplete(bool bSuccess)
{
    if (!bSuccess || !AsyncLoadingContext)
    {
        bIsLoading = false;
        return;
    }

    // 텍스처 설명 설정
    TextureDesc.Width = AsyncLoadingContext->Width;
    TextureDesc.Height = AsyncLoadingContext->Height;
    TextureDesc.Format = ETextureFormat::BGRA8;

    // 텍스처 생성
    if (AsyncLoadingContext->RenderHardware->IsDeviceReady())
    {
        CreateEmpty(
            AsyncLoadingContext->RenderHardware,
            TextureDesc,
            AsyncLoadingContext->PixelData.data()
        );
    }

    // 로딩 상태 업데이트
    bIsLoading = false;

    // 컨텍스트 정리
    AsyncLoadingContext.reset();
}

bool UTexture2D::CreateEmpty(IRenderHardware* RenderHardware, const FTextureDesc& Desc, const void* InitialData)
{
    if (!RenderHardware || !RenderHardware->GetDevice())
        return false;

    ID3D11Device* Device = RenderHardware->GetDevice();

    // 이전 리소스 해제
    Release();

    // 텍스처 설명 저장
    TextureDesc = Desc;

    // 텍스처 생성 설명 설정
    D3D11_TEXTURE2D_DESC TextureCreateDesc = {};
    TextureCreateDesc.Width = TextureDesc.Width;
    TextureCreateDesc.Height = TextureDesc.Height;
    TextureCreateDesc.MipLevels = TextureDesc.MipLevels;
    TextureCreateDesc.ArraySize = 1;
    TextureCreateDesc.Format = GetDXGIFormat(TextureDesc.Format);
    TextureCreateDesc.SampleDesc.Count = 1;
    TextureCreateDesc.SampleDesc.Quality = 0;
    TextureCreateDesc.Usage = TextureDesc.bDynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
    TextureCreateDesc.BindFlags = 0;

    // 바인딩 플래그 설정
    if (TextureDesc.bShaderResource)
        TextureCreateDesc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;

    if (TextureDesc.bRenderTarget)
        TextureCreateDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;

    if (TextureDesc.bUnorderedAccess)
        TextureCreateDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;

    // CPU 접근 플래그 설정
    TextureCreateDesc.CPUAccessFlags = TextureDesc.bDynamic ? D3D11_CPU_ACCESS_WRITE : 0;
    TextureCreateDesc.MiscFlags = 0;

    // 초기 데이터 설정
    D3D11_SUBRESOURCE_DATA InitData = {};

    if (InitialData)
    {
        InitData.pSysMem = InitialData;
        InitData.SysMemPitch = TextureDesc.Width * 4; // RGBA8 기준, 포맷에 맞게 수정 필요
        InitData.SysMemSlicePitch = 0;
    }

    // 텍스처 생성
    HRESULT hr = Device->CreateTexture2D(
        &TextureCreateDesc,
        InitialData ? &InitData : nullptr,
        &Texture
    );

    if (FAILED(hr))
        return false;

    // 셰이더 리소스 뷰 생성
    if (TextureDesc.bShaderResource)
    {
        D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
        SRVDesc.Format = TextureCreateDesc.Format;
        SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        SRVDesc.Texture2D.MipLevels = TextureDesc.MipLevels;
        SRVDesc.Texture2D.MostDetailedMip = 0;

        hr = Device->CreateShaderResourceView(Texture, &SRVDesc, &ShaderResourceView);

        if (FAILED(hr))
        {
            Release();
            return false;
        }
    }

    // 렌더 타겟 뷰 생성
    if (TextureDesc.bRenderTarget)
    {
        D3D11_RENDER_TARGET_VIEW_DESC RTVDesc = {};
        RTVDesc.Format = TextureCreateDesc.Format;
        RTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        RTVDesc.Texture2D.MipSlice = 0;

        hr = Device->CreateRenderTargetView(Texture, &RTVDesc, &RenderTargetView);

        if (FAILED(hr))
        {
            Release();
            return false;
        }
    }

    // UAV 생성
    if (TextureDesc.bUnorderedAccess)
    {
        D3D11_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
        UAVDesc.Format = TextureCreateDesc.Format;
        UAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
        UAVDesc.Texture2D.MipSlice = 0;

        hr = Device->CreateUnorderedAccessView(Texture, &UAVDesc, &UnorderedAccessView);

        if (FAILED(hr))
        {
            Release();
            return false;
        }
    }

    // 메모리 사용량 계산
    CalculateMemoryUsage();

    // 상태 업데이트
    bIsLoaded = true;
    bIsLoading = false;

    return true;
}


bool UTexture2D::UpdateData(ID3D11DeviceContext* DeviceContext, const void* Data, uint32_t Pitch)
{
    if (!DeviceContext || !Texture || !Data || !TextureDesc.bDynamic)
        return false;

    // 기본 피치 값 계산
    if (Pitch == 0)
    {
        Pitch = TextureDesc.Width * 4; // RGBA8 기준, 포맷에 맞게 수정 필요
    }

    // 리소스 매핑
    D3D11_MAPPED_SUBRESOURCE MappedResource;
    HRESULT hr = DeviceContext->Map(Texture, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);

    if (FAILED(hr))
        return false;

    // 데이터 복사
    BYTE* DestRow = static_cast<BYTE*>(MappedResource.pData);
    const BYTE* SrcRow = static_cast<const BYTE*>(Data);

    for (uint32_t y = 0; y < TextureDesc.Height; ++y)
    {
        memcpy(DestRow, SrcRow, Pitch);
        DestRow += MappedResource.RowPitch;
        SrcRow += Pitch;
    }

    // 리소스 언매핑
    DeviceContext->Unmap(Texture, 0);

    return true;
}

void UTexture2D::Clear(ID3D11DeviceContext* DeviceContext, const float Color[4])
{
    if (DeviceContext && RenderTargetView)
    {
        DeviceContext->ClearRenderTargetView(RenderTargetView, Color);
    }
}

DXGI_FORMAT UTexture2D::GetDXGIFormat(ETextureFormat Format) const
{
    switch (Format)
    {
        case ETextureFormat::RGBA8:
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        case ETextureFormat::BGRA8:
            return DXGI_FORMAT_B8G8R8A8_UNORM;
        case ETextureFormat::DXT1:
            return DXGI_FORMAT_BC1_UNORM;
        case ETextureFormat::DXT5:
            return DXGI_FORMAT_BC3_UNORM;
        case ETextureFormat::R8:
            return DXGI_FORMAT_R8_UNORM;
        case ETextureFormat::RG8:
            return DXGI_FORMAT_R8G8_UNORM;
        case ETextureFormat::RGBA16F:
            return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case ETextureFormat::RGBA32F:
            return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case ETextureFormat::DEPTH24_STENCIL8:
            return DXGI_FORMAT_D24_UNORM_S8_UINT;
        default:
            return DXGI_FORMAT_R8G8B8A8_UNORM;
    }
}

void UTexture2D::CalculateMemoryUsage()
{
    MemorySize = 0;

    if (!Texture)
        return;

    // 텍스처 메모리 계산
    uint32_t BytesPerPixel = 0;

    switch (TextureDesc.Format)
    {
        case ETextureFormat::RGBA8:
        case ETextureFormat::BGRA8:
            BytesPerPixel = 4;
            break;
        case ETextureFormat::DXT1:
            // DXT1은 4x4 픽셀 블록당 8바이트
            BytesPerPixel = 1 / 2; // 픽셀당 0.5바이트
            break;
        case ETextureFormat::DXT5:
            // DXT5는 4x4 픽셀 블록당 16바이트
            BytesPerPixel = 1; // 픽셀당 1바이트
            break;
        case ETextureFormat::R8:
            BytesPerPixel = 1;
            break;
        case ETextureFormat::RG8:
            BytesPerPixel = 2;
            break;
        case ETextureFormat::RGBA16F:
            BytesPerPixel = 8;
            break;
        case ETextureFormat::RGBA32F:
            BytesPerPixel = 16;
            break;
        case ETextureFormat::DEPTH24_STENCIL8:
            BytesPerPixel = 4;
            break;
    }

    // 각 밉맵 레벨의 메모리 계산
    uint32_t Width = TextureDesc.Width;
    uint32_t Height = TextureDesc.Height;

    for (uint32_t MipLevel = 0; MipLevel < TextureDesc.MipLevels; ++MipLevel)
    {
        // 각 밉맵 레벨의 크기 계산
        MemorySize += Width * Height * BytesPerPixel;

        // 다음 밉맵 레벨 크기
        Width = Math::Max(1u, Width / 2);
        Height = Math::Min(1u, Height / 2);
    }

    // 뷰 객체 추가 메모리 (추정치)
    if (ShaderResourceView) MemorySize += 64;
    if (RenderTargetView) MemorySize += 64;
    if (UnorderedAccessView) MemorySize += 64;
}