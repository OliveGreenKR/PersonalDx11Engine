#pragma once
#include "VertexFormat.h"
#include <vector>

// ���� �����͸� �����ϱ� ���� �����̳� ����ü
struct FVertexDataContainer
{
    std::vector<FVertexFormat> Vertices;  // ���� �迭
    std::vector<UINT> Indices;            // �ε��� �迭
};