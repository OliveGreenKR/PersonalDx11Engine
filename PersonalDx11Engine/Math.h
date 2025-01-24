#pragma once
#include <directxmath.h>
#include <cmath>
#include <algorithm>

using namespace DirectX;

const float PI = XM_PI;


const float KINDA_SMALL = 1e-4f; // 보통 용도
const float REALLY_SMALL = 1e-8f; // 정밀 계산 용도

// Forward declarations
struct Vector2;
struct Vector3;
struct Vector4;
struct Vector2I;
struct Vector3I;
struct Vector4I;
using Quaternion = Vector4;

namespace Math
{
	static float Lerp(const float min, const float max, const float alpha)
	{
		return min + alpha * (max - min);
	}

	static float DegreeToRad(float degree)
	{
		return degree * XM_PI / 180.0f;
	}
	static float RadToDegree(float rad)
	{
		return rad * 180.0f / XM_PI;
	}
	
	static float Clamp(float val, float min, float max)
	{
		return val < min ? min : (val > max ? max : val);
	}
}

#pragma region Vector
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
	Vector4() : XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f) {}
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
		if (L < KINDA_SMALL)
		{
			x = 0; y = 0; z = 0; w = 1.0f;
			return;
		}
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

	//Qauternion
	static Quaternion LookRotation(const Vector3& LookAtDirection, const Vector3& Up);
	static Quaternion Slerp(const Quaternion& Start, const Quaternion& End, float Factor);

	const static Quaternion Identity;
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

	//When Vector is too Small, be Zero
	void Normalize()
	{
		float L = Length();
		if (L < KINDA_SMALL)
		{
			x = 0; y = 0; z = 0;
			return;
		}
		if (std::abs(L-1) > KINDA_SMALL)
		{
			float InvL = 1.0f / L;
			x *= InvL;
			y *= InvL;
			z *= InvL;
		}
	}

	void SafeNormalize(Vector3& OutVec, const Vector3& ErrVec = Vector3::Zero)
	{
		float L = Length();
		if (L < KINDA_SMALL)
		{
			OutVec = ErrVec;
			return;
		}

		if (std::abs(L - 1) > KINDA_SMALL)
		{
			float InvL = 1.0f / L;
			OutVec.x = x * InvL;
			OutVec.y = y * InvL;
			OutVec.z = z * InvL;
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

	static Vector3 Lerp(const Vector3& Current, const Vector3& Dest, float Alpha)
	{
		return Vector3(
			Current.x + (Dest.x - Current.x) * Alpha,
			Current.y + (Dest.y - Current.y) * Alpha,
			Current.z + (Dest.z - Current.z) * Alpha
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

inline Quaternion Vector4::LookRotation(const Vector3& LookAt, const Vector3& Up)
{
	// 입력 벡터가 영벡터인 경우 체크
	if (LookAt.LengthSquared() < KINDA_SMALL)
	{
		return Quaternion(0, 0, 0, 1.0f);
	}
	// 1. 입력 벡터들을 XMVECTOR로 변환
	XMVECTOR vLookAt = XMVector3Normalize(XMLoadFloat3(&LookAt));
	XMVECTOR vUp = XMVector3Normalize(XMLoadFloat3(&Up));

	// 2. 직교 기저 벡터 계산
	// Forward = 정규화된 LookAt 벡터
	XMVECTOR vForward = vLookAt;

	// Right = Up × Forward (외적)
	XMVECTOR vRight = XMVector3Cross(vUp, vForward);

	// Right가 너무 작은 경우 (LookAt과 Up이 거의 평행할 때) 처리
	if (XMVector3LengthSq(vRight).m128_f32[0] < KINDA_SMALL)
	{
		// LookAt과 Up이 평행한 경우, 약간 다른 Up 벡터 사용
		vUp = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
		vRight = XMVector3Cross(vUp, vForward);

		// 여전히 너무 작으면 다른 축 시도
		if (XMVector3LengthSq(vRight).m128_f32[0] < KINDA_SMALL)
		{
			vUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
			vRight = XMVector3Cross(vUp, vForward);
		}
	}

	// Right 정규화
	vRight = XMVector3Normalize(vRight);

	// 실제 Up = Forward × Right
	vUp = XMVector3Cross(vForward, vRight);
	// Up은 정규화된 벡터들의 외적이므로 따로 정규화할 필요 없음

	// 3. 직교 기저 벡터들로 회전 행렬 생성
	XMMATRIX RotationMatrix(
		vRight,
		vUp,
		vForward,
		XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f)
	);

	// 4. 행렬을 쿼터니온으로 변환
	XMVECTOR quat = XMQuaternionRotationMatrix(RotationMatrix);

	// 5. 결과를 Quaternion 구조체에 저장
	Quaternion result;
	XMStoreFloat4(&result, quat);

	return result;
}

inline Quaternion Vector4::Slerp(const Quaternion& Start, const Quaternion& End, float Factor)
{
	// Factor를 0과 1 사이로 제한
	Factor = Math::Clamp(Factor, 0.0f, 1.0f);

	// XMVECTOR로 변환
	XMVECTOR Q0 = XMLoadFloat4(&Start);
	XMVECTOR Q1 = XMLoadFloat4(&End);

	// 두 쿼터니온 사이의 내적 계산
	float CosOmega = XMVectorGetX(XMQuaternionDot(Q0, Q1));

	// 내적이 음수인 경우, End 쿼터니온을 반전(더 짧은 호를 따라 보간)
	if (CosOmega < 0.0f)
	{
		Q1 = XMVectorNegate(Q1);
		CosOmega = -CosOmega;
	}

	float K0, K1;

	// 두 쿼터니온이 매우 가까운 경우 선형 보간 사용
	if (CosOmega > (1.0f-KINDA_SMALL))
	{
		// 선형 보간 계수 계산
		K0 = 1.0f - Factor;
		K1 = Factor;
	}
	else
	{
		// 구면 보간을 위한 각도와 사인 값 계산
		float Omega = std::acos(CosOmega);
		float SinOmega = std::sin(Omega);

		// 구면 보간 계수 계산
		K0 = std::sin((1.0f - Factor) * Omega) / SinOmega;
		K1 = std::sin(Factor * Omega) / SinOmega;
	}

	// 보간된 쿼터니온 계산
	XMVECTOR Result = XMVectorAdd(
		XMVectorScale(Q0, K0),
		XMVectorScale(Q1, K1)
	);

	// 결과 정규화
	Result = XMQuaternionNormalize(Result);

	// 결과를 Quaternion 구조체로 변환
	Quaternion ReturnValue;
	XMStoreFloat4(&ReturnValue, Result);

	return ReturnValue;
}

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

namespace Math
{
	//retrun Normalized Quat
	static Quaternion EulerToQuaternion(const Vector3& InEuler)
	{
		// 각도를 라디안으로 변환
		Vector3 RadAngles = InEuler * (PI / 180.0f);

		// 각 축별 회전에 대한 사인과 코사인 계산
		float cx = std::cos(RadAngles.x * 0.5f);
		float cy = std::cos(RadAngles.y * 0.5f);
		float cz = std::cos(RadAngles.z * 0.5f);
		float sx = std::sin(RadAngles.x * 0.5f);
		float sy = std::sin(RadAngles.y * 0.5f);
		float sz = std::sin(RadAngles.z * 0.5f);

		// XYZ 순서로 회전을 적용한 쿼터니온 생성
		Quaternion q;
		q.w = cx * cy * cz + sx * sy * sz;
		q.x = sx * cy * cz - cx * sy * sz;
		q.y = cx * sy * cz + sx * cy * sz;
		q.z = cx * cy * sz - sx * sy * cz;

		return q.GetNormalized();  // 수치 오차를 방지하기 위해 정규화
	}

	static Vector3 QuaternionToEuler(const Quaternion& q)
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

	static XMVECTOR Slerp(const XMVECTOR& Start, const XMVECTOR& End, float Factor)
	{
		Factor = Math::Clamp(Factor,0.0f, 1.0f);

		XMVECTOR Q0 = Start;
		XMVECTOR Q1 = End;

		XMVECTOR Result = XMQuaternionSlerp(Q0, Q1, Factor);
		return Result;
	}
	static Quaternion Slerp(const Quaternion& Start, const Quaternion& End, float Factor)
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

	static XMVECTOR GetRotationBetweenVectors(const XMVECTOR& target, const XMVECTOR& dest)
	{
		// 두 벡터를 정규화
		XMVECTOR v1 = XMVector3Normalize(target);
		XMVECTOR v2 = XMVector3Normalize(dest);

		float cosAngle = XMVectorGetX(XMVector3Dot(v1, v2));

		float threshold = 1.0f - KINDA_SMALL;

		//parallel
		if (cosAngle > threshold)
			return XMQuaternionIdentity();
		//counter
		if(cosAngle < -threshold)
		{
			XMVECTOR rotAxis = XMVector3Normalize(XMVector3Cross(v1, XMVectorSet(0, 1, 0, 0)));
			return XMQuaternionRotationAxis(rotAxis, XM_PI);
		}

		// 회전축과 각도로 쿼터니언 생성
		XMVECTOR rotAxis = XMVector3Normalize(XMVector3Cross(v1, v2));
		float angle = acos(cosAngle);
		return XMQuaternionRotationAxis(rotAxis, angle);
	}

	static Quaternion GetRotationBetweenVectors(const Vector3& target, const Vector3& dest)
	{
		XMVECTOR V1 = XMLoadFloat3(&target);
		XMVECTOR V2 = XMLoadFloat3(&dest);

		XMVECTOR vRotation = GetRotationBetweenVectors(V1, V2);
		
		Quaternion result;
		XMStoreFloat4(&result, vRotation);
		return result;
	}
}


#pragma region Plane
struct Plane : public Vector4
{
	Plane() = default;
	Plane(Vector4& plane) : Vector4(plane) {}
	Plane(float x, float y, float z, float w) : Vector4(x, y, z, w) {};

	void NormalizePlane()
	{
		Vector3 normal(*this);
		float L = normal.Length();
		if (::abs(1 - L) > KINDA_SMALL)
		{
			float InvL = 1.0f / L;
			x *= InvL;
			y *= InvL;
			z *= InvL;
			w *= InvL;
		}
	}

	float GetDistance(float x, float y, float z) const
	{
		XMVECTOR normal = XMLoadFloat4(this);
		XMVECTOR point = XMVectorSet(x, y, z, 1.0f);
		return XMVectorGetX(XMVector4Dot(normal, point));
	}

	float GetDistance(const Vector3& Point) const
	{
		return GetDistance(Point.x, Point.y, Point.z);
	}
	bool IsInFront(const Vector3& Point) const
	{
		return GetDistance(Point) >= 0.0f;
	}

};
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



