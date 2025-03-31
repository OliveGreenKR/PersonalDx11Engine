#pragma once
#include "RenderDefines.h"

//렌더링 상태 : 렌더링 방식을 정의
class IRenderState
{
public:
    virtual ~IRenderState() = default;

    // 상태 적용
    virtual void Apply(class ID3D11DeviceContext* Context) = 0;

    // 이전 상태로 복원
    virtual void Restore(class ID3D11DeviceContext* Context) = 0;

    virtual enum class  ERenderStateType GetType() const = 0;
};

