#pragma once

enum class ERenderStateType;

// 렌더 잡의 인터페이스
class IRenderJob
{
public:
    virtual ~IRenderJob() = default;
    virtual void Execute(FRenderContext& Context) = 0;
    virtual ERenderStateType GetStateType() const = 0;
};