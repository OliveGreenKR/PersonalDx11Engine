#pragma once
#include "Math.h"

//Implementation of standardVector
const Vector3 Vector3::Zero = Vector3(0, 0, 0);
const Vector3 Vector3::Up = Vector3(0, 1, 0);
const Vector3 Vector3::Forward = Vector3(0, 0, 1);
const Vector3 Vector3::Right = Vector3(1, 0, 0);
const Vector3 Vector3::One = Vector3(1, 1, 1);

const Quaternion Quaternion::Identity = Quaternion(0, 0, 0, 1.0f);