#pragma once
#pragma comment(lib, "user32")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")
#pragma comment(lib, "dxguid")

#include <d3d11.h>
#include <d3dcompiler.h>
#include <directxmath.h>
// IRenderHardware.h - 하드웨어 인터페이스
class IRenderHardware
{
public:
    virtual ~IRenderHardware() = default;

    virtual bool Initialize(HWND hWnd) = 0;
    virtual void Release() = 0;

    virtual void BeginFrame() = 0;
    virtual void EndFrame() = 0;

    // 하드웨어 접근자
    virtual ID3D11Device* GetDevice() = 0;
    virtual ID3D11DeviceContext* GetDeviceContext() = 0;

    // 비동기 작업용 추가 메서드
    virtual bool IsDeviceReady() = 0;
};