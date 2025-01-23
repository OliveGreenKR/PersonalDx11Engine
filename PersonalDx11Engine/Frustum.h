#pragma once
#include "Math.h"

struct FFrustum
{
    Plane Near;     // 근평면
    Plane Far;      // 원평면
    Plane Left;     // 좌평면
    Plane Right;    // 우평면
    Plane Top;      // 상평면
    Plane Bottom;   // 하평면

    void NormalizeAll()
    {
        Near.NormalizePlane();
        Far.NormalizePlane();
        Left.NormalizePlane();
        Right.NormalizePlane();
        Top.NormalizePlane();
        Bottom.NormalizePlane();
    }
};