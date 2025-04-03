#pragma once
#include <memory>
#include "RenderDefines.h"
#include "Math.h"


class IMaterial
{
public:
    virtual ~IMaterial() = default;
    
    //색상을 반환
    virtual Vector4 GetColor() const = 0;

    // 텍스처를 반환 (없을 경우 nullptr 가능)
    virtual class UTexture2D* GetTexture() const = 0;

    // 정점 쉐이더를 반환 (없을 경우 nullptr 가능)
    virtual class UVertexShader* GetVertexShader() const = 0;

    // 정점 쉐이더를 반환 (없을 경우 nullptr 가능)
    virtual class UPixelShader* GetPixelShader() const = 0;

    // 렌더링 상태를 반환
    virtual enum class ERenderStateType GetRenderState() const = 0;
};