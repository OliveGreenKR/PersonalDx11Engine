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
//// ���� ������ ȯ��
//CoInitialize(nullptr);
//
//// ��Ƽ������ ȯ��
//CoInitializeEx(nullptr, COINIT_MULTITHREADED);

//need to COM Exit
//CoUninitialize();


bool LoadTextureFromFile(ID3D11Device* Device, const wchar_t* FilePath, ID3D11ShaderResourceView** OutShaderResourceView)
{
    // WIC ���丮 ����
    IWICImagingFactory* WICFactory = nullptr;
    HRESULT hr = CoCreateInstance(
        CLSID_WICImagingFactory,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&WICFactory)
    );
    if (FAILED(hr)) return false;

    // ���ڴ� ����
    IWICBitmapDecoder* Decoder = nullptr;
    hr = WICFactory->CreateDecoderFromFilename(
        FilePath,
        nullptr,
        GENERIC_READ,
        WICDecodeMetadataCacheOnLoad,
        &Decoder
    );
    if (FAILED(hr)) { WICFactory->Release(); return false; }

    // ������ ȹ��
    IWICBitmapFrameDecode* Frame = nullptr;
    hr = Decoder->GetFrame(0, &Frame);
    if (FAILED(hr)) { Decoder->Release(); WICFactory->Release(); return false; }

    // BGRA �������� ��ȯ
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

    // �̹��� ũ�� ȹ��
    UINT Width, Height;
    Converter->GetSize(&Width, &Height);

    // �ؽ�ó ����
    D3D11_TEXTURE2D_DESC TextureDesc = {};
    TextureDesc.Width = Width;
    TextureDesc.Height = Height;
    TextureDesc.MipLevels = 1;
    TextureDesc.ArraySize = 1;
    TextureDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    TextureDesc.SampleDesc.Count = 1;
    TextureDesc.Usage = D3D11_USAGE_DEFAULT;
    TextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    // �ȼ� ������ �غ�
    UINT RowPitch = Width * 4;
    std::vector<BYTE> Buffer(RowPitch * Height);
    Converter->CopyPixels(nullptr, RowPitch, Buffer.size(), Buffer.data());

    D3D11_SUBRESOURCE_DATA InitData = {};
    InitData.pSysMem = Buffer.data();
    InitData.SysMemPitch = RowPitch;

    ID3D11Texture2D* Texture = nullptr;
    hr = Device->CreateTexture2D(&TextureDesc, &InitData, &Texture);
    if (FAILED(hr)) { Converter->Release(); Frame->Release(); Decoder->Release(); WICFactory->Release(); return false; }

    // SRV ����
    D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
    SRVDesc.Format = TextureDesc.Format;
    SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    SRVDesc.Texture2D.MipLevels = 1;

    hr = Device->CreateShaderResourceView(Texture, &SRVDesc, OutShaderResourceView);

    // ���ҽ� ����
    Texture->Release();
    Converter->Release();
    Frame->Release();
    Decoder->Release();
    WICFactory->Release();

    return SUCCEEDED(hr);
}