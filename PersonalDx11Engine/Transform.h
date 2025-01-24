#pragma once
#include "Math.h"

using namespace DirectX;

struct FTransform
{
	//Quaternion EulerToQuaternion(const Vector3& EulerAngles) const;
	//Vector3 QuaternionToEuler(const Quaternion& Quaternion) const;

	//{ Pitch, Yaw, Roll }
	void SetRotation(const Vector3& InEulerAngles);
	void SetRotation(Quaternion& InQuaternion);

	const Vector3 GetEulerRotation() const;
	const Quaternion GetQuarternionRotation() const;

	void AddRotation(const Vector3& InEulerAngles);
	void RotateAroundAxis(const Vector3& InAxis, float AngleDegrees);

	Vector3 Position = Vector3(0.0f, 0.0f, 0.0f);
	//radian angles, {Pitch,Yaw,Roll}
	Quaternion Rotation = Quaternion(0.0f, 0.0f, 0.0f, 1.0f);
	Vector3 Scale = Vector3(1.0f, 1.0f, 1.0f);
	Matrix GetModelingMatrix() const;

};