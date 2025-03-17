#pragma once
#include  <iostream>
#include "Transform.h"


//////////////
///  debug ///
//////////////
#if (defined(_DEBUG) || defined(DEBUG))
#define IS_DEBUG_MODE 1
#else
#define IS_DEBUG_MODE 0
#endif

#if (defined(_DEBUG) || defined(DEBUG))
#define LOG(format, ...) printf(format, ##__VA_ARGS__); printf("\n")
#else
#define LOG(format, ...) ((void)0)
#endif

namespace Debug
{
	static const char* ToString(const FTransform& InTransform)
	{
		static char buffer[128];

		snprintf(buffer, sizeof(buffer),
				 "Position : %.2f  %.2f  %.2f\n"
				 "Rotation : %.2f  %.2f  %.2f\n"
				 "Scale    : %.2f  %.2f  %.2f\n",
				 InTransform.Position.x, InTransform.Position.y, InTransform.Position.z,
				 InTransform.GetEulerRotation().x, InTransform.GetEulerRotation().y, InTransform.GetEulerRotation().z,
				 InTransform.Scale.x, InTransform.Scale.y, InTransform.Scale.z);
		return buffer;
	}

	static const char* ToString(const Vector3& InVector , const char* Name = "Vector")
	{
		static char buffer[128];

		snprintf(buffer, sizeof(buffer),
				 Name,
				 " : %.2f  %.2f  %.2f\n",
				 InVector.x, InVector.y, InVector.z);

		return buffer;
	}

}
