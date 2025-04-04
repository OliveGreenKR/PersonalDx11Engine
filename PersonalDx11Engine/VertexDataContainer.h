#pragma once
#include "VertexFormat.h"
#include <vector>
#include <cstdint>

// 정점 데이터를 저장하기 위한 컨테이너 구조체
struct FVertexDataContainer
{
    std::vector<FVertexFormat> Vertices;  // 정점 배열
    std::vector<uint32_t> Indices;            // 인덱스 배열
};