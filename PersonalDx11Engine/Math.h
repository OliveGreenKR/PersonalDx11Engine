#pragma once
#include <directxmath.h>

using namespace DirectX;

const float PI = XM_PI;

// Vector types
using Vector2 = DirectX::XMVECTOR;
using Vector3 = DirectX::XMFLOAT3;
using Vector4 = DirectX::XMFLOAT4;

// Matrix types
using Matrix = DirectX::XMMATRIX;
// not SIMD Matrix types
using Matrix36 = DirectX::XMFLOAT3X3;
// not SIMD Matrix types
using Matrix64 = DirectX::XMFLOAT4X4;

// Quaternion type
using Quat = DirectX::XMFLOAT4;    // Quaternion stored as XMFLOAT4

// SIMD optimized types
using VectorRegister = DirectX::XMVECTOR;

// Color type
using Color = DirectX::XMFLOAT4;   // RGBA stored as XMFLOAT4

// Plane type
using Plane = DirectX::XMFLOAT4;   // Plane equation stored as XMFLOAT4 (ax + by + cz + d)



namespace V3
{
	//Unit Vectors in DX Coord
	const static Vector3 Zero();
	const static Vector3 Up();
	const static Vector3 Forward();
	const static Vector3 Right();

	const static Vector3 Zero()
	{
		const static Vector3 vec(0, 0, 0);
		return vec;
	}
	const static Vector3 Up()
	{
		const static Vector3 vec(0, 1.0f, 0);
		return vec;
	}
	const static Vector3 Forward()
	{
		const static Vector3 vec(0, 0, 1.0f);
		return vec;
	}
	const static Vector3 Right()
	{
		const static Vector3 vec(1.0f, 0, 0);
		return vec;
	}
}

