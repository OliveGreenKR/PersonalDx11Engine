#pragma once
#include <directxmath.h>

using namespace DirectX;

// Vector types
using Vector2 = DirectX::XMFLOAT2;
using Vector3 = DirectX::XMFLOAT3;
using Vector4 = DirectX::XMFLOAT4;

// Matrix types
using Matrix = DirectX::XMMATRIX;
using Matrix3x3 = DirectX::XMFLOAT3X3;
using Matrix4x4 = DirectX::XMFLOAT4X4;

// Quaternion type
using Quat = DirectX::XMFLOAT4;    // Quaternion stored as XMFLOAT4

// SIMD optimized types
using VectorRegister = DirectX::XMVECTOR;
using Matrix44f = DirectX::XMMATRIX;

// Color type
using Color = DirectX::XMFLOAT4;   // RGBA stored as XMFLOAT4

// Plane type
using Plane = DirectX::XMFLOAT4;   // Plane equation stored as XMFLOAT4 (ax + by + cz + d)
