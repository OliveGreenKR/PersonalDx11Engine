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
};