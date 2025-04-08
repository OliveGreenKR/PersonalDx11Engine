#pragma once
#pragma comment(lib, "user32")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")
#pragma comment(lib, "dxguid")

#include <d3d11.h>
#include <d3dcompiler.h> 
#include <directxmath.h>

#include <memory>
#include "RenderDataInterface.h"

// 렌더링 상태 타입
enum class ERenderStateType
{
    Default,            //렌더러 설정에 따라 렌더링
    Solid,
    Wireframe,
    // 필요한 상태 추가
};

struct FRenderJob
{
    class IRenderData* RenderData = nullptr;
    ERenderStateType RenderState = ERenderStateType::Default;   
};