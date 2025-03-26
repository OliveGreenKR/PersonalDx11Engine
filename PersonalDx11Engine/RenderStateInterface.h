#pragma once
#pragma comment(lib, "user32")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")
#pragma comment(lib, "dxguid")

#include <d3d11.h>
#include <d3dcompiler.h> 
#include <directxmath.h>

enum class ERenderStateType
{
    None,
    Solid,
    Wireframe,
    // 필요한 상태 추가
};

class IRenderState
{
public:
    virtual ~IRenderState() = default;

    // 상태 적용
    virtual void Apply(ID3D11DeviceContext* Context) = 0;

    // 이전 상태로 복원
    virtual void Restore(ID3D11DeviceContext* Context) = 0;

    virtual ERenderStateType GetType() const = 0;
};

