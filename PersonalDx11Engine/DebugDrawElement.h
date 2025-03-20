#pragma once
#include "Math.h"
#include <vector>
#include <memory>
#include <ctype.h>

using UINT = uint32_t;

// 디버그 드로우 요소의 기본 클래스
class FDebugDrawElement
{
public:
    struct FDebugVertex
    {
        Vector3 Position;
        Vector2 TexCoord;  // 호환성을 위해 유지 (0,0)으로 초기화
    };

protected:
    float Duration;         // 표시 지속 시간 (초)
    float CurrentTime;      // 현재 경과 시간
    Vector4 Color;          // 색상
    std::vector<FDebugVertex> Vertices;   // 정점 데이터
    std::vector<UINT> Indices;            // 인덱스 데이터

public:
    FDebugDrawElement(const Vector4& InColor, float InDuration = 0.0f)
        : Duration(InDuration)
        , CurrentTime(0.0f)
        , Color(InColor)
    {
    }

    virtual ~FDebugDrawElement() = default;

    // 지속 시간 업데이트
    void Update(float DeltaTime)
    {
        CurrentTime += DeltaTime;
    }

    // 수명이 다했는지 확인
    bool IsExpired() const
    {
        return Duration > 0.0f && CurrentTime >= Duration;
    }

    // 색상 getter
    const Vector4& GetColor() const { return Color; }

    // 버텍스 및 인덱스 액세스 함수
    const std::vector<FDebugVertex>& GetVertices() const { return Vertices; }
    const std::vector<UINT>& GetIndices() const { return Indices; }

    // 가상 함수: 드로우 요소 초기화
    virtual void Initialize() = 0;
};