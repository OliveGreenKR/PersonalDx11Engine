#pragma once
#include "Math.h"

// 기본 정점 형식 (위치와 텍스트좌표)
struct alignas(16) FVertexFormat
{
    Vector3 Position;  // 정점 위치
    Vector2 TexCoord;
    Vector3 Padding;
};