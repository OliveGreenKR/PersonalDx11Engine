#pragma once
#include "Math.h"

using namespace DirectX;

struct FTransform
{
	Vector3 Position = Vector3(0.0f, 0.0f, 0.0f);
	Vector3 Rotation = Vector3(0.0f, 0.0f, 0.0f);
	Vector3 Scale = Vector3(1.0f, 1.0f, 1.0f);

	Matrix GetModelingMatrix() const;
};