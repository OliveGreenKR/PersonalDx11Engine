#pragma once
#pragma comment(lib, "user32")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")
#pragma comment(lib, "dxguid")

#include <d3d11.h>
#include <d3dcompiler.h>
#include <directxmath.h>

class IRenderState
{
public:
    virtual ~IRenderState() = default;

    // ���� ����
    virtual void Apply(ID3D11DeviceContext* Context) = 0;

    // ���� ���·� ����
    virtual void Restore(ID3D11DeviceContext* Context) = 0;
};

