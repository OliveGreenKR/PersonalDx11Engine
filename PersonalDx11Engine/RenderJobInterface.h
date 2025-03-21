#pragma once

enum class ERenderStateType;

// ���� ���� �������̽�
class IRenderJob
{
public:
    virtual ~IRenderJob() = default;
    virtual void Execute(FRenderContext& Context) = 0;
    virtual ERenderStateType GetStateType() const = 0;
};