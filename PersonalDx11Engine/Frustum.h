#pragma once
#include "Math.h"

struct FFrustum
{
    Plane Near;     // �����
    Plane Far;      // �����
    Plane Left;     // �����
    Plane Right;    // �����
    Plane Top;      // �����
    Plane Bottom;   // �����

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