#pragma once
#include "Math.h"
#include "Delegate.h"

using namespace DirectX;

struct FTransform
{
	Vector3 Position = Vector3(0.0f, 0.0f, 0.0f);
	Quaternion Rotation = Quaternion(0.0f, 0.0f, 0.0f, 1.0f);
	Vector3 Scale = Vector3(1.0f, 1.0f, 1.0f);

	const Vector3 GetEulerRotation() const { return Math::QuaternionToEuler(Rotation); }

	void RotateAroundAxis(const Vector3& InAxis, float AngleDegrees);

	Matrix GetTranslationMatrix() const;
	Matrix GetScaleMatrix() const;
	Matrix GetRotationMatrix() const;
	Matrix GetModelingMatrix() const;

	static FTransform  InterpolateTransform(const FTransform& Start, const FTransform& End, float Alpha);
};