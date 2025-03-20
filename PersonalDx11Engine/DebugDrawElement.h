#pragma once
#include "Math.h"
#include <vector>
#include <memory>
#include <ctype.h>

using UINT = uint32_t;

// ����� ��ο� ����� �⺻ Ŭ����
class FDebugDrawElement
{
public:
    struct FDebugVertex
    {
        Vector3 Position;
        Vector2 TexCoord;  // ȣȯ���� ���� ���� (0,0)���� �ʱ�ȭ
    };

protected:
    float Duration;         // ǥ�� ���� �ð� (��)
    float CurrentTime;      // ���� ��� �ð�
    Vector4 Color;          // ����
    std::vector<FDebugVertex> Vertices;   // ���� ������
    std::vector<UINT> Indices;            // �ε��� ������

public:
    FDebugDrawElement(const Vector4& InColor, float InDuration = 0.0f)
        : Duration(InDuration)
        , CurrentTime(0.0f)
        , Color(InColor)
    {
    }

    virtual ~FDebugDrawElement() = default;

    // ���� �ð� ������Ʈ
    void Update(float DeltaTime)
    {
        CurrentTime += DeltaTime;
    }

    // ������ ���ߴ��� Ȯ��
    bool IsExpired() const
    {
        return Duration > 0.0f && CurrentTime >= Duration;
    }

    // ���� getter
    const Vector4& GetColor() const { return Color; }

    // ���ؽ� �� �ε��� �׼��� �Լ�
    const std::vector<FDebugVertex>& GetVertices() const { return Vertices; }
    const std::vector<UINT>& GetIndices() const { return Indices; }

    // ���� �Լ�: ��ο� ��� �ʱ�ȭ
    virtual void Initialize() = 0;
};