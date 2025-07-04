#pragma once
#include "Math.h"
#include "Delegate.h"

using namespace DirectX;

struct FTransform
{
	FTransform() = default;
	FTransform(Vector3 InPos, Quaternion InRot, Vector3 InScale)
		: Position(InPos), Rotation(InRot), Scale(InScale)
	{

	}

	Vector3 Position = Vector3(0.0f, 0.0f, 0.0f);
	Quaternion Rotation = Quaternion(0.0f, 0.0f, 0.0f, 1.0f);
	Vector3 Scale = Vector3(1.0f, 1.0f, 1.0f);

	const Vector3 GetEulerRotation() const { return Math::QuaternionToEuler(Rotation); }

	void RotateAroundAxis(const Vector3& InAxis, float AngleDegrees);

	Matrix GetTranslationMatrix() const;
	Matrix GetScaleMatrix() const;
	Matrix GetRotationMatrix() const;
	Matrix GetModelingMatrix() const;

	static FTransform  Lerp(const FTransform& Start, const FTransform& End, float Alpha);
	static bool IsEqual(const FTransform& A, const FTransform& B, const float Epsilon = TRANSFORM_EPSILON);

	static constexpr float TRANSFORM_EPSILON = KINDA_SMALLER;

	//의미있는 위치 벡터 인지 확인 (Epsilon이상의 크기)
	static bool IsValidPosition(const Vector3& InVector);
	//의미있는 스케일 벡터 인지 확인 (Epsilon이상의 크기)
	static bool IsValidScale(const Vector3& InVector);
	//QuatA와 QuatB를 비교하여 의미있는 변화량인지 확인 (Quat 의 DotProduct를 이용)
	static bool IsValidRotation(const Quaternion & QuatA, const Quaternion & QuatB = Quaternion::Identity());
};