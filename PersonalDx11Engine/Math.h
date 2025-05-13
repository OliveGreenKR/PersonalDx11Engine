#pragma once
#include <directxmath.h>
#include <cmath>
#include <algorithm>

using namespace DirectX;

constexpr float PI = XM_PI;
constexpr float KINDA_SMALL = 1e-4f; // 보통 용도
constexpr float KINDA_SMALLER = 1e-6f; // 정밀 계산 용도

// Forward declarations
struct Vector2;
struct Vector3;
struct Vector4;
struct Vector2I;
struct Vector3I;
struct Vector4I;
using Quaternion = Vector4;

namespace XMVector
{
	inline static const DirectX::XMVECTOR XMUp()
	{
		return XMVectorSet(0, 1, 0, 1);
	}

	inline static const DirectX::XMVECTOR XMForward()
	{
		return XMVectorSet(0, 0, 1, 1);
	}

	inline static const DirectX::XMVECTOR XMRight()
	{
		return XMVectorSet(1, 0, 0, 1);
	}

	inline static const DirectX::XMVECTOR XMZero()
	{
		return XMVectorSet(0, 0, 0, 1);
	}
}

namespace Math
{
	inline static constexpr float Lerp(const float min, const float max, const float alpha)
	{
		return min + alpha * (max - min);
	}

	inline static constexpr float DegreeToRad(float degree)
	{
		return degree * XM_PI / 180.0f;
	}

	inline static constexpr float RadToDegree(float rad)
	{
		return rad * 180.0f / XM_PI;
	}

	inline static constexpr float Clamp(float val, float min, float max)
	{
		return val < min ? min : (val > max ? max : val);
	}
}

#pragma region Vector
// Float vector declarations
struct Vector4 : public DirectX::XMFLOAT4
{
	constexpr Vector4() noexcept : XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f) {}
	constexpr Vector4(float X, float Y, float Z, float W) noexcept : XMFLOAT4(X, Y, Z, W) {}
	constexpr explicit Vector4(const DirectX::XMFLOAT4& V) : XMFLOAT4(V) {}

	constexpr explicit Vector4(const XMFLOAT3& Vec) : XMFLOAT4(Vec.x, Vec.y, Vec.z, 1.0f) {};//w=1.0f
	constexpr explicit Vector4(const XMFLOAT2& Vec) : XMFLOAT4(Vec.x, Vec.y, 0.0f, 1.0f) {};// z = 0.0f, w = 1.0f

	constexpr bool operator==(const Vector4& Other)
	{
		return x == Other.x &&
			y == Other.y &&
			z == Other.z &&
			w == Other.w;
	}

	constexpr Vector4& operator+=(const Vector4& Other)
	{
		x += Other.x;
		y += Other.y;
		z += Other.z;
		w += Other.w;
		return *this;
	}

	constexpr Vector4& operator-=(const Vector4& Other)
	{
		x -= Other.x;
		y -= Other.y;
		z -= Other.z;
		w -= Other.w;
		return *this;
	}

	constexpr Vector4& operator*=(float Scalar)
	{
		x *= Scalar;
		y *= Scalar;
		z *= Scalar;
		w *= Scalar;
		return *this;
	}

	constexpr Vector4& operator/=(float Scalar)
	{
		float InvScalar = 1.0f / Scalar;
		x *= InvScalar;
		y *= InvScalar;
		z *= InvScalar;
		w *= InvScalar;
		return *this;
	}

	constexpr Vector4 operator+(const Vector4& Other) const { return Vector4(x + Other.x, y + Other.y, z + Other.z, w + Other.w); }
	constexpr Vector4 operator-(const Vector4& Other) const { return Vector4(x - Other.x, y - Other.y, z - Other.z, w - Other.w); }
	constexpr Vector4 operator*(float Scalar) const { return Vector4(x * Scalar, y * Scalar, z * Scalar, w * Scalar); }
	constexpr Vector4 operator/(float Scalar) const
	{
		float InvScalar = 1.0f / Scalar;
		return Vector4(x * InvScalar, y * InvScalar, z * InvScalar, w * InvScalar);
	}

	float Length() const { return sqrt(x * x + y * y + z * z + w * w); }
	constexpr float LengthSquared() const { return x * x + y * y + z * z + w * w; }

	void Normalize();

	Vector4 GetNormalized() const;
	static float Dot(const Vector4& A, const Vector4& B);

	//Qauternion
	static Quaternion LookRotation(const Vector3 & LookAtDirection, const Vector3& Up);

	static constexpr Quaternion Identity() { return Quaternion(0, 0, 0, 1); }
};

struct Vector2 : public DirectX::XMFLOAT2
{
	constexpr Vector2() noexcept : XMFLOAT2(0.0f, 0.0f) {}
	constexpr Vector2(float X, float Y) noexcept : XMFLOAT2(X, Y) {}
	constexpr explicit Vector2(const DirectX::XMFLOAT2& V) : XMFLOAT2(V) {}
	constexpr explicit Vector2(const XMFLOAT4& Vec) : XMFLOAT2(Vec.x, Vec.y) {}
	constexpr explicit Vector2(const XMFLOAT3& Vec) : XMFLOAT2(Vec.x, Vec.y) {};

	constexpr Vector2& operator+=(const Vector2& Other)
	{
		x += Other.x;
		y += Other.y;
		return *this;
	}

	constexpr Vector2& operator-=(const Vector2& Other)
	{
		x -= Other.x;
		y -= Other.y;
		return *this;
	}

	constexpr Vector2& operator*=(float Scalar)
	{
		x *= Scalar;
		y *= Scalar;
		return *this;
	}

	constexpr Vector2& operator/=(float Scalar)
	{
		float InvScalar = 1.0f / Scalar;
		x *= InvScalar;
		y *= InvScalar;
		return *this;
	}

	constexpr Vector2 operator+(const Vector2& Other) const { return Vector2(x + Other.x, y + Other.y); }
	constexpr Vector2 operator-(const Vector2& Other) const { return Vector2(x - Other.x, y - Other.y); }
	constexpr Vector2 operator*(float Scalar) const { return Vector2(x * Scalar, y * Scalar); }
	constexpr Vector2 operator/(float Scalar) const
	{
		float InvScalar = 1.0f / Scalar;
		return Vector2(x * InvScalar, y * InvScalar);
	}


	float Length() const { return sqrt(x * x + y * y); }
	constexpr float LengthSquared() const { return x * x + y * y; }

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
	constexpr Vector3() noexcept : XMFLOAT3(0.0f, 0.0f, 0.0f) {}
	constexpr Vector3(float X, float Y, float Z) noexcept : XMFLOAT3(X, Y, Z) {}
	constexpr explicit Vector3(const DirectX::XMFLOAT3& V) : XMFLOAT3(V) {}
	constexpr explicit Vector3(const XMFLOAT4& Vec) : XMFLOAT3(Vec.x, Vec.y, Vec.z) {}
	constexpr explicit Vector3(const XMFLOAT2& Vec) : XMFLOAT3(Vec.x, Vec.y, 0.0f) {}


	static constexpr Vector3 Zero() { return Vector3(0.0f, 0.0f, 0.0f); }
	static constexpr Vector3 Up() { return Vector3(0.0f, 1.0f, 0.0f); }
	static constexpr Vector3 Forward() { return Vector3(0.0f, 0.0f, 1.0f); }
	static constexpr Vector3 Right() { return Vector3(1.0f, 0.0f, 0.0f); }
	static constexpr Vector3 One() { return Vector3(1.0f, 1.0f, 1.0f); }

	// Assignment operators
	constexpr Vector3& operator=(const Vector4& Vec)
	{
		x = Vec.x;
		y = Vec.y;
		z = Vec.z;
		return *this;
	}

	constexpr Vector3& operator=(const Vector2& Vec)
	{
		x = Vec.x;
		y = Vec.y;
		z = 0.0f;
		return *this;
	}

	constexpr Vector3& operator+=(const Vector3& Other)
	{
		x += Other.x;
		y += Other.y;
		z += Other.z;
		return *this;
	}

	constexpr Vector3& operator-=(const Vector3& Other)
	{
		x -= Other.x;
		y -= Other.y;
		z -= Other.z;
		return *this;
	}

	constexpr Vector3& operator*=(float Scalar)
	{
		x *= Scalar;
		y *= Scalar;
		z *= Scalar;
		return *this;
	}

	constexpr Vector3& operator/=(float Scalar)
	{
		float InvScalar = 1.0f / Scalar;
		x *= InvScalar;
		y *= InvScalar;
		z *= InvScalar;
		return *this;
	}

	// Arithmetic operators
	constexpr Vector3 operator+(const Vector3& Other) const { return Vector3(x + Other.x, y + Other.y, z + Other.z); }
	constexpr Vector3 operator-(const Vector3& Other) const { return Vector3(x - Other.x, y - Other.y, z - Other.z); }
	constexpr Vector3 operator*(float Scalar) const { return Vector3(x * Scalar, y * Scalar, z * Scalar); }
	constexpr Vector3 operator/(float Scalar) const
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

	// Utility functions
	float Length() const { return sqrt(x * x + y * y + z * z); }
	constexpr float LengthSquared() const { return x * x + y * y + z * z; }

	//When Vector is too Small, be Zero
	void Normalize();

	void SafeNormalize(Vector3& OutVec, const Vector3& ErrVec = Vector3::Zero());

	Vector3 GetNormalized() const;

	// Static utility functions
	static float Dot(const Vector3& A, const Vector3& B);

	static Vector3 Cross(const Vector3& A, const Vector3& B);

	static Vector3 Min(const Vector3& A, const Vector3& B);

	static Vector3 Max(const Vector3& A, const Vector3& B);

	static Vector3 Clamp(const Vector3& Value, const Vector3& Min, const Vector3& Max);
};

// Global operators for scalar multiplication
inline constexpr Vector2 operator*(float Scalar, const Vector2& Vec) { return Vec * Scalar; }
inline constexpr Vector3 operator*(float Scalar, const Vector3& Vec) { return Vec * Scalar; }
inline constexpr Vector4 operator*(float Scalar, const Vector4& Vec) { return Vec * Scalar; }

#pragma endregion



namespace Math
{
	inline static constexpr float Max(const float a, const float b) {
		return a > b ? a : b;
	}
	inline constexpr float Min(const float a, const float b) {
		return a > b ? b : a;
	}

	inline static XMVECTOR RotateAroundAxis(XMVECTOR InAxis, float RadianAngle)
	{
		//회전축 정규화
		XMVECTOR NormalizedAxis = XMVector3Normalize(InAxis);
		return XMQuaternionRotationNormal(NormalizedAxis, RadianAngle);
	}
	//retrun Normalized Quat
	//retrun Normalized Quat
	inline static const Quaternion EulerToQuaternion(const Vector3& InEuler)
	{
		// 각도를 라디안으로 변환
		XMVECTOR RadianAngles = XMVectorScale(
			XMLoadFloat3(&InEuler),
			XM_PI / 180.0f
		);

		// 각 축별 회전에 대한 독립적 계산
		XMVECTOR PitchRotation = RotateAroundAxis(
			XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f),  // X축
			XMVectorGetX(RadianAngles)            // 피치 각도
		);

		XMVECTOR YawRotation = RotateAroundAxis(
			XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f),  // Y축
			XMVectorGetY(RadianAngles)            // 요 각도
		);

		XMVECTOR RollRotation = RotateAroundAxis(
			XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f),  // Z축
			XMVectorGetZ(RadianAngles)            // 롤 각도
		);

		XMVECTOR RotationVec = XMQuaternionMultiply(
			XMQuaternionMultiply(PitchRotation, YawRotation), RollRotation);

		RotationVec = XMQuaternionNormalize(RotationVec);
		Quaternion result;
		XMStoreFloat4(&result, RotationVec);
		return result;
	}

	inline static Vector3 QuaternionToEuler(const Quaternion& q)
	{
		Vector3 EulerAngles;
		// 쿼터니온을 오일러 각으로 변환
		// atan2를 사용하여 -180도에서 180도 사이의 각도를 얻음

		// Pitch (x-axis rotation)
		float sinr_cosp = 2.0f * (q.w * q.x + q.y * q.z);
		float cosr_cosp = 1.0f - 2.0f * (q.x * q.x + q.y * q.y);
		EulerAngles.x = std::atan2(sinr_cosp, cosr_cosp);

		// Yaw (y-axis rotation)
		float sinp = 2.0f * (q.w * q.y - q.z * q.x);
		if (std::abs(sinp) >= 1.0f)
		{
			// 90도에서의 특이점 처리
			EulerAngles.y = std::copysign(PI / 2.0f, sinp);
		}
		else
		{
			EulerAngles.y = std::asin(sinp);
		}

		// Roll (z-axis rotation)
		float siny_cosp = 2.0f * (q.w * q.z + q.x * q.y);
		float cosy_cosp = 1.0f - 2.0f * (q.y * q.y + q.z * q.z);
		EulerAngles.z = std::atan2(siny_cosp, cosy_cosp);

		// 라디안을 도로 변환
		return EulerAngles * (180.0f / PI);
	}

	inline static XMVECTOR Lerp(const XMVECTOR& Start, const XMVECTOR& End, float Alpha)
	{
		return XMVectorLerp(Start, End, Alpha);
	}

	inline static Vector3 Lerp(const Vector3& Current, const Vector3& Dest, float Alpha)
	{
		XMVECTOR V0 = XMLoadFloat3(&Current);
		XMVECTOR V1 = XMLoadFloat3(&Dest);
		XMVECTOR Result = Lerp(V0, V1, Alpha);
		Vector3 ResultVector;
		XMStoreFloat3(&ResultVector, Result);
		return ResultVector;
	}


	inline static XMVECTOR Slerp(const XMVECTOR& Start, const XMVECTOR& End, float Factor)
	{
		Factor = Math::Clamp(Factor, 0.0f, 1.0f);

		XMVECTOR Q0 = Start;
		XMVECTOR Q1 = End;

		XMVECTOR Result = XMQuaternionSlerp(Q0, Q1, Factor);
		return Result;
	}

	inline static Quaternion Slerp(const Quaternion& Start, const Quaternion& End, float Factor)
	{
		// SIMD 연산을 위해 XMVECTOR 변환
		XMVECTOR Q0 = XMLoadFloat4(&Start);
		XMVECTOR Q1 = XMLoadFloat4(&End);

		// SIMD를 활용한 보간 계산
		XMVECTOR Result = Slerp(Q0, Q1, Factor);

		// 결과 변환 및 반환
		Quaternion ResultFloat4;
		XMStoreFloat4(&ResultFloat4, Result);
		return ResultFloat4;
	}

	inline static XMVECTOR GetRotationBetweenVectors(const XMVECTOR& target, const XMVECTOR& dest)
	{
		// 상수 정의
		static constexpr float kParallelThreshold = 1.0f - KINDA_SMALL;

		// 벡터 정규화
		XMVECTOR v1 = XMVector3Normalize(target);
		XMVECTOR v2 = XMVector3Normalize(dest);

		// 코사인 각도 계산
		XMVECTOR cosAngle = XMVector3Dot(v1, v2);
		cosAngle = XMVectorMin(
			XMVectorMax(cosAngle, XMVectorReplicate(-1.0f)),
			XMVectorReplicate(1.0f)
		);

		// 평행한 경우 처리
		if (XMVectorGetX(cosAngle) > kParallelThreshold)
		{
			return XMQuaternionIdentity();
		}

		// 반대 방향인 경우 처리
		if (XMVectorGetX(cosAngle) < -kParallelThreshold)
		{
			// XMVector 네임스페이스의 상수 활용
			XMVECTOR rotAxis = XMVector3Normalize(XMVector3Cross(v1, XMVector::XMUp()));

			// 회전축이 0인 경우, Right 벡터 사용
			if (XMVector3Equal(rotAxis, XMVector::XMZero()))
			{
				rotAxis = XMVector3Normalize(XMVector3Cross(v1, XMVector::XMRight()));
			}

			return XMQuaternionRotationAxis(rotAxis, XM_PI);
		}

		// 일반적인 경우의 회전 계산
		XMVECTOR rotAxis = XMVector3Normalize(XMVector3Cross(v1, v2));
		float angle = XMVectorGetX(XMVectorACos(cosAngle));

		return XMQuaternionRotationAxis(rotAxis, angle);
	}

	inline static Quaternion GetRotationBetweenVectors(const Vector3& target, const Vector3& dest)
	{
		XMVECTOR V1 = XMLoadFloat3(&target);
		XMVECTOR V2 = XMLoadFloat3(&dest);

		XMVECTOR vRotation = GetRotationBetweenVectors(V1, V2);

		Quaternion result;
		XMStoreFloat4(&result, vRotation);
		return result;
	}

	inline static XMVECTOR GetRotationVBetweenVectors(const Vector3& target, const Vector3& dest)
	{
		XMVECTOR V1 = XMLoadFloat3(&target);
		XMVECTOR V2 = XMLoadFloat3(&dest);

		return GetRotationBetweenVectors(V1, V2);
	}
}

#pragma region Plane
struct Plane : public Vector4
{
	Plane() = default;
	Plane(Vector4& plane) : Vector4(plane) {}
	Plane(float x, float y, float z, float w) : Vector4(x, y, z, w) {};

	void NormalizePlane();

	float GetDistance(float x, float y, float z) const;

	float GetDistance(const Vector3& Point) const;
	bool IsInFront(const Vector3& Point) const;

};
#pragma endregion

#pragma region Aligned Vectors
// MatrIx types
using Matrix = DirectX::XMMATRIX;

//Aligned Data Struct for Vector
union alignas(16) AVector
{
	XMFLOAT4 Vector;			// 16바이트 (float4)
	float floats[4];			// 16바이트 (4개의 float)
	char byte[16];				// 16바이트 (바이트 단위 접근)
};

union alignas(16) AMatrix
{
	XMMATRIX Matrix;
	AVector Vectors[4];
};

struct alignas(16) AMatrix128
{
	AMatrix Matrices[2];
};

struct alignas(16) AMatrix192
{
	AMatrix Matrices[3];
};

struct alignas(16) AMatrix256
{
	AMatrix Matrices[4];
};
#pragma endregion





