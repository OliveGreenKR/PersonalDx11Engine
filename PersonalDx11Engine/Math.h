#pragma once
#include <directxmath.h>

using namespace DirectX;

const float PI = XM_PI;

const float KINDA_SMALL = 1e-4f; // 보통 용도
const float REALLY_SMALL = 1e-8f; // 정밀 계산 용도

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

// Plane type - normalized normal
using Plane = DirectX::XMFLOAT4;   

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

namespace Math
{
	static float Clamp(float val, float min, float max)
	{
		return val < min ? min : (val > max ? max : val);
	}

	static float GetDistanceFromPlane(const Vector3& Point, const Plane& Plane)
	{
		XMVECTOR planeNormal = XMLoadFloat4(&Plane);
		XMVECTOR pointVec = XMVectorSet(Point.x, Point.y, Point.z, 1.0f);
		return XMVectorGetX(XMVector4Dot(planeNormal, pointVec));
	}

	static bool IsFront(const Plane& Plane, const Vector3& Point)
	{
		float distance = GetDistanceFromPlane(Point, Plane);
		return distance >= 0.0f;
	}

}



