#pragma once
#pragma once
#include "Math.h"
#include "ImGui/imgui.h"
#include "Color.h"
#include <vector>
#include <memory>

class UCamera;  // ī�޶� Ŭ���� ���� ����

// ����� ����� ��Ҹ� ���� �⺻ ����ü
struct FDebugDrawElement
{
    float Duration;
    float CurrentTime;
    bool bPersistent;  // true�� ��� Duration ����

    FDebugDrawElement(float InDuration = 0.3f)
        : Duration(InDuration)
        , CurrentTime(0.0f)
        , bPersistent(false)
    {}

    virtual ~FDebugDrawElement() = default;
    virtual void Draw(UCamera* Camera) = 0;
    virtual bool IsExpired() const { return !bPersistent && CurrentTime >= Duration; }
};

// ȭ��ǥ ����� ���
struct FDebugDrawArrow : public FDebugDrawElement
{
    Vector3 Start;
    Vector3 Direction;
    float Length;
    float Size;
    Vector4 Color;

    FDebugDrawArrow(
        const Vector3& InStart,
        const Vector3& InDirection,
        float InLength,
        float InSize,
        const Vector4& InColor,
        float InDuration = 0.3f)
        : FDebugDrawElement(InDuration)
        , Start(InStart)
        , Direction(InDirection)
        , Length(InLength)
        , Size(InSize)
        , Color(InColor)
    {}

    virtual void Draw(UCamera* Camera) override;
};

// ����� ����� �Ŵ���
class UDebugDrawManager
{
private:
    UDebugDrawManager() = default;
    ~UDebugDrawManager() = default;

    // ����/�̵� �� ���� ������ ����
    UDebugDrawManager(const UDebugDrawManager&) = delete;
    UDebugDrawManager& operator=(const UDebugDrawManager&) = delete;
    UDebugDrawManager(UDebugDrawManager&&) = delete;
    UDebugDrawManager& operator=(UDebugDrawManager&&) = delete;

public:
    static UDebugDrawManager* Get()
    {
        static UDebugDrawManager* Instance = []() {
            UDebugDrawManager* Manager = new UDebugDrawManager();
            return Manager;
            }();
        return Instance;
    }

    void DrawArrow(
        const Vector3& Start,
        const Vector3& Direction,
        float Length,
        float Size,
        const Vector4& Color,
        float Duration = 0.3f);

    void Tick(const float DeltaTime);

    void DrawAll(UCamera* Camera);

private:
    std::vector<std::unique_ptr<FDebugDrawElement>> Elements;
};

