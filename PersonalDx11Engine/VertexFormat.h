#pragma once
#include "Math.h"

// �⺻ ���� ���� (��ġ�� �ؽ�Ʈ��ǥ)
struct alignas(16) FVertexFormat
{
    Vector3 Position;  // ���� ��ġ
    Vector2 TexCoord;
    Vector3 Padding;
};