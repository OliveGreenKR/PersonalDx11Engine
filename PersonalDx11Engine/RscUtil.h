#pragma once

#pragma comment(lib, "user32")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")
#pragma comment(lib, "dxguid")
#pragma comment(lib, "windowscodecs")

#include <d3d11.h>
#include <d3dcompiler.h>
#include <directxmath.h>

#include <wincodec.h>
#include <vector>

//need to COM Init
//// 단일 스레드 환경
//CoInitialize(nullptr);
//
//// 멀티스레드 환경
//CoInitializeEx(nullptr, COINIT_MULTITHREADED);

//need to COM Exit
//CoUninitialize();

inline bool LoadTextureDataFromFile(const wchar_t* FilePath, BYTE** OutPixelData, UINT* OutWidth, UINT* OutHeight, UINT* OutRowPitch)
{
    // WIC 팩토리 생성
    IWICImagingFactory* WICFactory = nullptr;
    HRESULT hr = CoCreateInstance(
        CLSID_WICImagingFactory,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&WICFactory)
    );
    if (FAILED(hr)) return false;

    // 디코더 생성
    IWICBitmapDecoder* Decoder = nullptr;
    hr = WICFactory->CreateDecoderFromFilename(
        FilePath,
        nullptr,
        GENERIC_READ,
        WICDecodeMetadataCacheOnLoad,
        &Decoder
    );
    if (FAILED(hr)) { WICFactory->Release(); return false; }

    // 프레임 획득
    IWICBitmapFrameDecode* Frame = nullptr;
    hr = Decoder->GetFrame(0, &Frame);
    if (FAILED(hr)) { Decoder->Release(); WICFactory->Release(); return false; }

    // BGRA 포맷으로 변환
    IWICFormatConverter* Converter = nullptr;
    hr = WICFactory->CreateFormatConverter(&Converter);
    if (FAILED(hr)) { Frame->Release(); Decoder->Release(); WICFactory->Release(); return false; }

    hr = Converter->Initialize(
        Frame,
        GUID_WICPixelFormat32bppBGRA,
        WICBitmapDitherTypeNone,
        nullptr,
        0.0f,
        WICBitmapPaletteTypeCustom
    );
    if (FAILED(hr)) { Converter->Release(); Frame->Release(); Decoder->Release(); WICFactory->Release(); return false; }

    // 이미지 크기 획득
    Converter->GetSize(OutWidth, OutHeight);
    *OutRowPitch = *OutWidth * 4;

    // 픽셀 데이터를 위한 메모리 할당 (호출자가 해제 책임)
    *OutPixelData = new BYTE[*OutRowPitch * *OutHeight];

    // 픽셀 데이터 복사
    Converter->CopyPixels(nullptr, *OutRowPitch, *OutRowPitch * *OutHeight, *OutPixelData);

    // 리소스 정리
    Converter->Release();
    Frame->Release();
    Decoder->Release();
    WICFactory->Release();

    return true;
}

inline bool LoadTextureFromFile(ID3D11Device* Device, const wchar_t* FilePath, ID3D11ShaderResourceView** OutShaderResourceView)
{
    HRESULT hr;
    BYTE* PixelData = nullptr;
    UINT Width = 0, Height = 0, RowPitch = 0;

    // 픽셀 데이터 로드
    if (!LoadTextureDataFromFile(FilePath, &PixelData, &Width, &Height, &RowPitch))
        return false;

    // 텍스처 생성을 위한 정보 구성
    D3D11_TEXTURE2D_DESC TextureDesc = {};
    TextureDesc.Width = Width;
    TextureDesc.Height = Height;
    TextureDesc.MipLevels = 1;
    TextureDesc.ArraySize = 1;
    TextureDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    TextureDesc.SampleDesc.Count = 1;
    TextureDesc.SampleDesc.Quality = 0;
    TextureDesc.Usage = D3D11_USAGE_DEFAULT;
    TextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    TextureDesc.CPUAccessFlags = 0;
    TextureDesc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA InitData = {};
    InitData.pSysMem = PixelData;
    InitData.SysMemPitch = RowPitch;
    InitData.SysMemSlicePitch = 0;

    // 텍스처 생성
    ID3D11Texture2D* Texture = nullptr;
    hr = Device->CreateTexture2D(&TextureDesc, &InitData, &Texture);

    // 메모리 해제 (데이터가 GPU로 복사되었으므로 더 이상 필요 없음)
    delete[] PixelData;

    if (FAILED(hr))
        return false;

    // SRV 생성
    D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
    SRVDesc.Format = TextureDesc.Format;
    SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    SRVDesc.Texture2D.MipLevels = 1;

    hr = Device->CreateShaderResourceView(Texture, &SRVDesc, OutShaderResourceView);

    // 텍스처 해제 (SRV가 참조를 유지함)
    Texture->Release();

    return SUCCEEDED(hr);
}