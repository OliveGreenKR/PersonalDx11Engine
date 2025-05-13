#include "Math.h"

void Plane::NormalizePlane()
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

float Plane::GetDistance(float x, float y, float z) const
{
	XMVECTOR normal = XMLoadFloat4(this);
	XMVECTOR point = XMVectorSet(x, y, z, 1.0f);
	return XMVectorGetX(XMVector4Dot(normal, point));
}

float Plane::GetDistance(const Vector3& Point) const
{
	return GetDistance(Point.x, Point.y, Point.z);
}

bool Plane::IsInFront(const Vector3& Point) const
{
	return GetDistance(Point) >= 0.0f;
}

//When Vector is too Small, be Zero
void Vector3::Normalize()
{
	float L = Length();
	if (L < KINDA_SMALL)
	{
		x = 0; y = 0; z = 0;
		return;
	}
	if (std::abs(L - 1) > KINDA_SMALL)
	{
		float InvL = 1.0f / L;
		x *= InvL;
		y *= InvL;
		z *= InvL;
	}
}

void Vector3::SafeNormalize(Vector3& OutVec, const Vector3& ErrVec)
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

Vector3 Vector3::GetNormalized() const
{
	Vector3 Result = *this;
	Result.Normalize();
	return Result;
}

// Static utility functions
float Vector3::Dot(const Vector3& A, const Vector3& B)
{
	XMVECTOR vA, vB;
	vA = XMLoadFloat3(&A);
	vB = XMLoadFloat3(&B);

	XMVECTOR vResult = XMVector3Dot(vA, vB);
	return XMVectorGetX(vResult);
}

Vector3 Vector3::Cross(const Vector3& A, const Vector3& B)
{
	XMVECTOR vA, vB;
	vA = XMLoadFloat3(&A);
	vB = XMLoadFloat3(&B);

	XMVECTOR vResult = XMVector3Cross(vA, vB);
	Vector3 Result;
	XMStoreFloat3(&Result, vResult);
	return Result;
}

Vector3 Vector3::Min(const Vector3& A, const Vector3& B)
{
	return Vector3(
		std::min<float>(A.x, B.x),
		std::min<float>(A.y, B.y),
		std::min<float>(A.z, B.z)
	);
}

Vector3 Vector3::Max(const Vector3& A, const Vector3& B)
{
	return Vector3(
		std::max<float>(A.x, B.x),
		std::max<float>(A.y, B.y),
		std::max<float>(A.z, B.z)
	);
}

Vector3 Vector3::Clamp(const Vector3& Value, const Vector3& Min, const Vector3& Max)
{
	return Vector3(
		Math::Clamp(Value.x, Min.x, Max.x),
		Math::Clamp(Value.y, Min.y, Max.y),
		Math::Clamp(Value.z, Min.z, Max.z)
	);
}

//Qauternion
Quaternion Quaternion::LookRotation(const Vector3& LookAtDirection, const Vector3& Up)
{
	// 입력 LookAt 벡터가 영벡터에 가까운 경우 처리
	if (LookAtDirection.LengthSquared() < KINDA_SMALL * KINDA_SMALL)
	{
		// 유효하지 않은 LookAt 입력이므로 기본 회전 (identity) 반환
		return Quaternion(0, 0, 0, 1.0f);
	}

	XMVECTOR vLookAt = XMLoadFloat3(&LookAtDirection);
	XMVECTOR vUp = XMLoadFloat3(&Up);

	// Forward 벡터는 정규화된 LookAt 벡터입니다.
	XMVECTOR vForward = XMVector3Normalize(vLookAt);

	// Right 벡터를 계산합니다. 초기 Right는 Up과 Forward의 외적입니다.
	// 이 벡터는 Forward에 직교하지만, 입력 Up 방향과는 다를 수 있습니다.
	XMVECTOR vRight = XMVector3Cross(vUp, vForward);

	// Up과 Forward가 거의 평행하여 Right 벡터가 매우 작을 때 처리합니다.
	// 이 상황에서는 Forward에 직교하는 다른 기준 축을 사용하여 Right를 다시 계산해야 합니다.
	if (XMVector3LengthSq(vRight).m128_f32[0] < KINDA_SMALL)
	{
		// Forward 벡터에 직교하는 안정적인 대체 축을 선택합니다.
		// 만약 Forward가 Z축과 거의 평행하다면 (0,0,1)과 외적 시 영벡터가 될 수 있으므로 (0,1,0) Y축을 사용합니다.
		// 그 외의 경우에는 (0,0,1) Z축을 사용해도 안전합니다.
		XMVECTOR vFallbackAxis = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f); // 기본 대체 축: Z축
		// Forward 벡터와 Z축의 내적 절대값이 1에 가까운지 확인 (거의 평행하다는 의미)
		// 1e-4f는 부동 소수점 비교를 위한 작은 허용 오차입니다.
		if (XMVectorGetX(XMVectorAbs(XMVector3Dot(vForward, XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f)))) > 1.0f - KINDA_SMALL)
		{
			vFallbackAxis = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f); // Forward가 Z축에 가까우면 Y축을 사용
		}

		// 선택된 대체 축과 Forward 벡터의 외적으로 새로운 Right 벡터를 계산합니다.
		// 이 Right 벡터는 Forward에 직교하며 영벡터가 아님이 보장됩니다 (FallbackAxis가 Forward와 평행하지 않도록 선택했으므로).
		vRight = XMVector3Cross(vFallbackAxis, vForward);

		// 예외적인 상황: 만약 대체 축 선택 로직에도 문제가 있거나 (매우 드물지만)
		// Right 벡터가 여전히 너무 작다면 유효한 회전 행렬 생성이 불가능합니다.
		if (XMVector3LengthSq(vRight).m128_f32[0] < KINDA_SMALL)
		{
			// 최후의 안전 장치: 유효한 입력을 얻지 못했으므로 기본 회전 (identity) 반환
			return Quaternion(0, 0, 0, 1.0f);
		}
	}

	// Right 벡터를 정규화하여 단위 벡터로 만듭니다.
	vRight = XMVector3Normalize(vRight);

	// 최종 Up 벡터는 Forward 벡터와 (새로 계산되거나 정규화된) Right 벡터에 모두 직교해야 합니다.
	// Forward × Right 외적을 통해 얻어진 벡터는 원래 입력 Up 방향과 가장 가깝고 Forward에도 직교하는 벡터입니다.
	XMVECTOR vUpActual = XMVector3Cross(vForward, vRight);
	// vUpActual은 이미 직교하는 두 단위 벡터의 외적이므로 별도의 정규화가 필요 없습니다.

	// 계산된 직교 기저 벡터들 (Right, UpActual, Forward)로 회전 행렬을 생성합니다.
	// 행렬의 열 벡터는 각각 로컬 X, Y, Z 축에 해당합니다.
	XMMATRIX RotationMatrix(
		vRight,      // X 기저 벡터 (오브젝트의 로컬 Right 방향)
		vUpActual,   // Y 기저 벡터 (오브젝트의 로컬 Up 방향)
		vForward,    // Z 기저 벡터 (오브젝트의 로컬 Forward 방향)
		XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f) // 변환(Translation) 부분은 0
	);

	// 생성된 회전 행렬로부터 쿼터니온을 계산합니다.
	XMVECTOR quat = XMQuaternionRotationMatrix(RotationMatrix);

	// 계산된 쿼터니온 결과를 Quaternion 구조체에 저장합니다.
	Quaternion result;
	XMStoreFloat4(&result, quat);

	return result;
}

void Vector4::Normalize()
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

Vector4 Vector4::GetNormalized() const
{
	Vector4 Result = *this;
	Result.Normalize();
	return Result;
}

float Vector4::Dot(const Vector4& A, const Vector4& B)
{
	return A.x * B.x + A.y * B.y + A.z * B.z + A.w * B.w;
}

