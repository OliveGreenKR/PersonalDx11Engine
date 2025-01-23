#pragma once
#include <directxmath.h>
#include <cmath>
#include <algorithm>

using namespace DirectX;

const float PI = XM_PI;

const float KINDA_SMALL = 1e-4f; // 보통 용도
const float REALLY_SMALL = 1e-8f; // 정밀 계산 용도

namespace Math
{
	static float Clamp(float val, float min, float max)
	{
		return val < min ? min : (val > max ? max : val);
	}

	//static float GetDistanceFromPlane(const Vector3& Point, const Plane& Plane)
	//{
	//	XMVECTOR planeNormal = XMLoadFloat4(&Plane);
	//	XMVECTOR pointVec = XMVectorSet(Point.x, Point.y, Point.z, 1.0f);
	//	return XMVectorGetX(XMVector4Dot(planeNormal, pointVec));
	//}

	//static bool IsFront(const Plane& Plane, const Vector3& Point)
	//{
	//	float distance = GetDistanceFromPlane(Point, Plane);
	//	return distance >= 0.0f;
	//}
}

#pragma region Vector
// Forward declarations
struct Vector2;
struct Vector3;
struct Vector4;
struct Vector2I;
struct Vector3I;
struct Vector4I;

// Integer vector declarations
struct Vector2I
{
	int32_t x;
	int32_t y;

	Vector2I() : x(0), y(0) {}
	Vector2I(int32_t X, int32_t Y) : x(X), y(Y) {}

	Vector2I& operator+=(const Vector2I& Other)
	{
		x += Other.x;
		y += Other.y;
		return *this;
	}

	Vector2I& operator-=(const Vector2I& Other)
	{
		x -= Other.x;
		y -= Other.y;
		return *this;
	}

	Vector2I& operator*=(int32_t Scalar)
	{
		x *= Scalar;
		y *= Scalar;
		return *this;
	}

	Vector2I operator+(const Vector2I& Other) const { return Vector2I(x + Other.x, y + Other.y); }
	Vector2I operator-(const Vector2I& Other) const { return Vector2I(x - Other.x, y - Other.y); }
	Vector2I operator*(int32_t Scalar) const { return Vector2I(x * Scalar, y * Scalar); }

	// Float vector conversion
	static Vector2 Create(const Vector2I& IntVec);
};

struct Vector3I
{
	int32_t x;
	int32_t y;
	int32_t z;

	Vector3I() : x(0), y(0), z(0) {}
	Vector3I(int32_t X, int32_t Y, int32_t Z) : x(X), y(Y), z(Z) {}

	Vector3I& operator+=(const Vector3I& Other)
	{
		x += Other.x;
		y += Other.y;
		z += Other.z;
		return *this;
	}

	Vector3I& operator-=(const Vector3I& Other)
	{
		x -= Other.x;
		y -= Other.y;
		z -= Other.z;
		return *this;
	}

	Vector3I& operator*=(int32_t Scalar)
	{
		x *= Scalar;
		y *= Scalar;
		z *= Scalar;
		return *this;
	}

	Vector3I operator+(const Vector3I& Other) const { return Vector3I(x + Other.x, y + Other.y, z + Other.z); }
	Vector3I operator-(const Vector3I& Other) const { return Vector3I(x - Other.x, y - Other.y, z - Other.z); }
	Vector3I operator*(int32_t Scalar) const { return Vector3I(x * Scalar, y * Scalar, z * Scalar); }

	// Float vector conversion
	static Vector3 Create(const Vector3I& IntVec);
};

struct Vector4I
{
	int32_t x;
	int32_t y;
	int32_t z;
	int32_t w;

	Vector4I() : x(0), y(0), z(0), w(0) {}
	Vector4I(int32_t X, int32_t Y, int32_t Z, int32_t W) : x(X), y(Y), z(Z), w(W) {}

	Vector4I& operator+=(const Vector4I& Other)
	{
		x += Other.x;
		y += Other.y;
		z += Other.z;
		w += Other.w;
		return *this;
	}

	Vector4I& operator-=(const Vector4I& Other)
	{
		x -= Other.x;
		y -= Other.y;
		z -= Other.z;
		w -= Other.w;
		return *this;
	}

	Vector4I& operator*=(int32_t Scalar)
	{
		x *= Scalar;
		y *= Scalar;
		z *= Scalar;
		w *= Scalar;
		return *this;
	}

	Vector4I operator+(const Vector4I& Other) const { return Vector4I(x + Other.x, y + Other.y, z + Other.z, w + Other.w); }
	Vector4I operator-(const Vector4I& Other) const { return Vector4I(x - Other.x, y - Other.y, z - Other.z, w - Other.w); }
	Vector4I operator*(int32_t Scalar) const { return Vector4I(x * Scalar, y * Scalar, z * Scalar, w * Scalar); }

	// Float vector conversion
	static Vector4 Create(const Vector4I& IntVec);
};

// Float vector declarations
struct Vector4 : public DirectX::XMFLOAT4
{
	Vector4() : XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f) {}
	Vector4(float X, float Y, float Z, float W) : XMFLOAT4(X, Y, Z, W) {}
	explicit Vector4(const DirectX::XMFLOAT4& V) : XMFLOAT4(V) {}

	// Implicit conversions from lower dimensions
	Vector4(const Vector3& Vec);//w=1.0f
	Vector4(const Vector2& Vec);// z = 0.0f, w = 1.0f

	Vector4& operator+=(const Vector4& Other)
	{
		x += Other.x;
		y += Other.y;
		z += Other.z;
		w += Other.w;
		return *this;
	}

	Vector4& operator-=(const Vector4& Other)
	{
		x -= Other.x;
		y -= Other.y;
		z -= Other.z;
		w -= Other.w;
		return *this;
	}

	Vector4& operator*=(float Scalar)
	{
		x *= Scalar;
		y *= Scalar;
		z *= Scalar;
		w *= Scalar;
		return *this;
	}

	Vector4& operator/=(float Scalar)
	{
		float InvScalar = 1.0f / Scalar;
		x *= InvScalar;
		y *= InvScalar;
		z *= InvScalar;
		w *= InvScalar;
		return *this;
	}

	Vector4 operator+(const Vector4& Other) const { return Vector4(x + Other.x, y + Other.y, z + Other.z, w + Other.w); }
	Vector4 operator-(const Vector4& Other) const { return Vector4(x - Other.x, y - Other.y, z - Other.z, w - Other.w); }
	Vector4 operator*(float Scalar) const { return Vector4(x * Scalar, y * Scalar, z * Scalar, w * Scalar); }
	Vector4 operator/(float Scalar) const
	{
		float InvScalar = 1.0f / Scalar;
		return Vector4(x * InvScalar, y * InvScalar, z * InvScalar, w * InvScalar);
	}

	// Integer vector conversion
	static Vector4I CreateInt(const Vector4& Vec)
	{
		return Vector4I(
			static_cast<int32_t>(Vec.x),
			static_cast<int32_t>(Vec.y),
			static_cast<int32_t>(Vec.z),
			static_cast<int32_t>(Vec.w)
		);
	}

	float Length() const { return sqrt(x * x + y * y + z * z + w * w); }
	float LengthSquared() const { return x * x + y * y + z * z + w * w; }

	void Normalize()
	{
		float L = Length();
		if (L > 0)
		{
			float InvL = 1.0f / L;
			x *= InvL;
			y *= InvL;
			z *= InvL;
			w *= InvL;
		}
	}

	Vector4 GetNormalized() const
	{
		Vector4 Result = *this;
		Result.Normalize();
		return Result;
	}

	static float Dot(const Vector4& A, const Vector4& B)
	{
		return A.x * B.x + A.y * B.y + A.z * B.z + A.w * B.w;
	}
};

struct Vector2 : public DirectX::XMFLOAT2
{
	Vector2() : XMFLOAT2(0.0f, 0.0f) {}
	Vector2(float X, float Y) : XMFLOAT2(X, Y) {}
	explicit Vector2(const DirectX::XMFLOAT2& V) : XMFLOAT2(V) {}

	// Implicit conversions from higher dimensions
	Vector2(const Vector4& Vec) : XMFLOAT2(Vec.x, Vec.y) {}
	Vector2(const Vector3& Vec);

	Vector2& operator+=(const Vector2& Other)
	{
		x += Other.x;
		y += Other.y;
		return *this;
	}

	Vector2& operator-=(const Vector2& Other)
	{
		x -= Other.x;
		y -= Other.y;
		return *this;
	}

	Vector2& operator*=(float Scalar)
	{
		x *= Scalar;
		y *= Scalar;
		return *this;
	}

	Vector2& operator/=(float Scalar)
	{
		float InvScalar = 1.0f / Scalar;
		x *= InvScalar;
		y *= InvScalar;
		return *this;
	}

	Vector2 operator+(const Vector2& Other) const { return Vector2(x + Other.x, y + Other.y); }
	Vector2 operator-(const Vector2& Other) const { return Vector2(x - Other.x, y - Other.y); }
	Vector2 operator*(float Scalar) const { return Vector2(x * Scalar, y * Scalar); }
	Vector2 operator/(float Scalar) const
	{
		float InvScalar = 1.0f / Scalar;
		return Vector2(x * InvScalar, y * InvScalar);
	}

	// Integer vector conversion
	static Vector2I CreateInt(const Vector2& Vec)
	{
		return Vector2I(static_cast<int32_t>(Vec.x), static_cast<int32_t>(Vec.y));
	}

	float Length() const { return sqrt(x * x + y * y); }
	float LengthSquared() const { return x * x + y * y; }

	void Normalize()
	{
		float L = Length();
		if (L > 0)
		{
			float InvL = 1.0f / L;
			x *= InvL;
			y *= InvL;
		}
	}

	Vector2 GetNormalized() const
	{
		Vector2 Result = *this;
		Result.Normalize();
		return Result;
	}

	static float Dot(const Vector2& A, const Vector2& B)
	{
		return A.x * B.x + A.y * B.y;
	}
};

struct Vector3 : public DirectX::XMFLOAT3
{
	Vector3() : XMFLOAT3(0.0f, 0.0f, 0.0f) {}
	Vector3(float X, float Y, float Z) : XMFLOAT3(X, Y, Z) {}
	explicit Vector3(const DirectX::XMFLOAT3& V) : XMFLOAT3(V) {}

	// Implicit conversions
	Vector3(const Vector4& Vec) : XMFLOAT3(Vec.x, Vec.y, Vec.z) {}
	Vector3(const Vector2& Vec) : XMFLOAT3(Vec.x, Vec.y, 0.0f) {}

	//static
	static const Vector3 Zero;
	static const Vector3 Up;
	static const Vector3 Forward;
	static const Vector3 Right;

	// Assignment operators
	Vector3& operator=(const Vector4& Vec)
	{
		x = Vec.x;
		y = Vec.y;
		z = Vec.z;
		return *this;
	}

	Vector3& operator=(const Vector2& Vec)
	{
		x = Vec.x;
		y = Vec.y;
		z = 0.0f;
		return *this;
	}

	Vector3& operator+=(const Vector3& Other)
	{
		x += Other.x;
		y += Other.y;
		z += Other.z;
		return *this;
	}

	Vector3& operator-=(const Vector3& Other)
	{
		x -= Other.x;
		y -= Other.y;
		z -= Other.z;
		return *this;
	}

	Vector3& operator*=(float Scalar)
	{
		x *= Scalar;
		y *= Scalar;
		z *= Scalar;
		return *this;
	}

	Vector3& operator/=(float Scalar)
	{
		float InvScalar = 1.0f / Scalar;
		x *= InvScalar;
		y *= InvScalar;
		z *= InvScalar;
		return *this;
	}

	// Arithmetic operators
	Vector3 operator+(const Vector3& Other) const { return Vector3(x + Other.x, y + Other.y, z + Other.z); }
	Vector3 operator-(const Vector3& Other) const { return Vector3(x - Other.x, y - Other.y, z - Other.z); }
	Vector3 operator*(float Scalar) const { return Vector3(x * Scalar, y * Scalar, z * Scalar); }
	Vector3 operator/(float Scalar) const
	{
		float InvScalar = 1.0f / Scalar;
		return Vector3(x * InvScalar, y * InvScalar, z * InvScalar);
	}

	// Unary operators
	Vector3 operator-() const { return Vector3(-x, -y, -z); }

	// Comparison operators
	bool operator==(const Vector3& Other) const
	{
		return x == Other.x && y == Other.y && z == Other.z;
	}

	bool operator!=(const Vector3& Other) const
	{
		return !(*this == Other);
	}

	// Integer vector conversion
	static Vector3I CreateInt(const Vector3& Vec)
	{
		return Vector3I(
			static_cast<int32_t>(Vec.x),
			static_cast<int32_t>(Vec.y),
			static_cast<int32_t>(Vec.z)
		);
	}

	// Utility functions
	float Length() const { return sqrt(x * x + y * y + z * z); }
	float LengthSquared() const { return x * x + y * y + z * z; }

	void Normalize()
	{
		float L = Length();
		if (L > 0)
		{
			float InvL = 1.0f / L;
			x *= InvL;
			y *= InvL;
			z *= InvL;
		}
	}

	Vector3 GetNormalized() const
	{
		Vector3 Result = *this;
		Result.Normalize();
		return Result;
	}

	// Static utility functions
	static float Dot(const Vector3& A, const Vector3& B)
	{
		return A.x * B.x + A.y * B.y + A.z * B.z;
	}

	static Vector3 Cross(const Vector3& A, const Vector3& B)
	{
		return Vector3(
			A.y * B.z - A.z * B.y,
			A.z * B.x - A.x * B.z,
			A.x * B.y - A.y * B.x
		);
	}

	static Vector3 Min(const Vector3& A, const Vector3& B)
	{
		return Vector3(
			std::min<float>(A.x, B.x),
			std::min<float>(A.y, B.y),
			std::min<float>(A.z, B.z)
		);
	}

	static Vector3 Max(const Vector3& A, const Vector3& B)
	{
		return Vector3(
			std::max<float>(A.x, B.x),
			std::max<float>(A.y, B.y),
			std::max<float>(A.z, B.z)
		);
	}

	static Vector3 Clamp(const Vector3& Value, const Vector3& Min, const Vector3& Max)
	{
		return Vector3(
			Math::Clamp(Value.x, Min.x, Max.x),
			Math::Clamp(Value.y, Min.y, Max.y),
			Math::Clamp(Value.z, Min.z, Max.z)
		);
	}

	static Vector3 Lerp(const Vector3& A, const Vector3& B, float Alpha)
	{
		return Vector3(
			A.x + (B.x - A.x) * Alpha,
			A.y + (B.y - A.y) * Alpha,
			A.z + (B.z - A.z) * Alpha
		);
	}
};

inline Vector2::Vector2(const Vector3& Vec) : XMFLOAT2(Vec.x, Vec.y) {}


// Implementation of Create functions for integer
inline Vector2 Vector2I::Create(const Vector2I& IntVec)
{
	return Vector2(
		static_cast<float>(IntVec.x),
		static_cast<float>(IntVec.y)
	);
}

inline Vector3 Vector3I::Create(const Vector3I& IntVec)
{
	return Vector3(
		static_cast<float>(IntVec.x),
		static_cast<float>(IntVec.y),
		static_cast<float>(IntVec.z)
	);
}

inline Vector4 Vector4I::Create(const Vector4I& IntVec)
{
	return Vector4(
		static_cast<float>(IntVec.x),
		static_cast<float>(IntVec.y),
		static_cast<float>(IntVec.z),
		static_cast<float>(IntVec.w)
	);
}

// Implementation of Vector4 conversion constructors
inline Vector4::Vector4(const Vector3& Vec) : XMFLOAT4(Vec.x, Vec.y, Vec.z, 1.0f) {}
inline Vector4::Vector4(const Vector2& Vec) : XMFLOAT4(Vec.x, Vec.y, 0.0f, 1.0f) {}

// Global operators for scalar multiplication
inline Vector2 operator*(float Scalar, const Vector2& Vec) { return Vec * Scalar; }
inline Vector3 operator*(float Scalar, const Vector3& Vec) { return Vec * Scalar; }
inline Vector4 operator*(float Scalar, const Vector4& Vec) { return Vec * Scalar; }

// Utility functions for common vector operations
inline float Distance(const Vector2& A, const Vector2& B)
{
	Vector2 Diff = A - B;
	return Diff.Length();
}

inline float Distance(const Vector3& A, const Vector3& B)
{
	Vector3 Diff = A - B;
	return Diff.Length();
}

inline float Distance(const Vector4& A, const Vector4& B)
{
	Vector4 Diff = A - B;
	return Diff.Length();
}

inline float DistanceSquared(const Vector2& A, const Vector2& B)
{
	Vector2 Diff = A - B;
	return Diff.LengthSquared();
}

inline float DistanceSquared(const Vector3& A, const Vector3& B)
{
	Vector3 Diff = A - B;
	return Diff.LengthSquared();
}

inline float DistanceSquared(const Vector4& A, const Vector4& B)
{
	Vector4 Diff = A - B;
	return Diff.LengthSquared();
}
#pragma endregion


// MatrIx types
using Matrix = DirectX::XMMATRIX;
// not SIMD Matrix types
using Matrix36 = DirectX::XMFLOAT3X3;
// not SIMD Matrix types
using Matrix64 = DirectX::XMFLOAT4X4;

// Quaternion type
using Quat = Vector4;    // Quaternion stored as XMFLOAT4

// SIMD optimized types
using VectorRegister = DirectX::XMVECTOR;

// Color type
using Color = Vector4;   // RGBA stored as XMFLOAT4

// Plane type - normalized normal
using Plane = Vector4;   





