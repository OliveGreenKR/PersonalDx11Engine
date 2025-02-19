#pragma once
#include "Transform.h"

namespace Utils
{
#if defined(_DEBUG) || defined(DEBUG)  // DEBUG 또는 _DEBUG가 정의되어 있을 때만 컴파일
	const char* ToString(const FTransform& InTransform)
	{
		static char buffer[128];

		snprintf(buffer, sizeof(buffer),
				 "Position : %.2f  %.2f  %.2f\n"
				 "Rotation : %.2f  %.2f  %.2f\n"
				 "Scale    : %.2f  %.2f  %.2f\n",
				 InTransform.GetPosition().x, InTransform.GetPosition().y, InTransform.GetPosition().z,
				 InTransform.GetEulerRotation().x, InTransform.GetEulerRotation().y, InTransform.GetEulerRotation().z,
				 InTransform.GetScale().x, InTransform.GetScale().y, InTransform.GetScale().z);

		return buffer;
	}

	void DrawDebugArrow()
	{

	}

#else
	const char* ToString(const FTransform& InTransform)
	{
		return "";
	}
#endif
}

