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
