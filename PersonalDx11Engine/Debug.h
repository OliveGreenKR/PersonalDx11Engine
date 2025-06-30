#pragma once
#include  <iostream>
#include <filesystem>
#include <cstring>   
#include "Transform.h"
#include "ConsoleManager.h"

//////////////
///  debug ///
//////////////
#if (defined(_DEBUG) || defined(DEBUG))
#define IS_DEBUG_MODE 1
#else
#define IS_DEBUG_MODE 0
#endif

namespace Debug
{
	static const char* ToString(const FTransform& InTransform)
	{
		char buffer[128];

		snprintf(buffer, sizeof(buffer),
				 "Position : %.2f  %.2f  %.2f\n"
				 "Rotation : %.2f  %.2f  %.2f\n"
				 "Scale    : %.2f  %.2f  %.2f\n",
				 InTransform.Position.x, InTransform.Position.y, InTransform.Position.z,
				 InTransform.GetEulerRotation().x, InTransform.GetEulerRotation().y, InTransform.GetEulerRotation().z,
				 InTransform.Scale.x, InTransform.Scale.y, InTransform.Scale.z);
		return buffer;
	}

	static const char* ToString(const Vector3& InVector , const char* Name = "")
	{
		char buffer[128];
		if (Name)
		{
			snprintf(buffer, sizeof(buffer),
					 "%s : %.2f  %.2f  %.2f\n",
					 Name,
					 InVector.x, InVector.y, InVector.z);
		}
		else
		{
			snprintf(buffer, sizeof(buffer),
					 "%.2f  %.2f  %.2f\n",
					 InVector.x, InVector.y, InVector.z);
		}

		return buffer;
	}

}

#pragma region LOG

// 파일명만 추출하는 매크로 
#define __FILENAME__ (std::strrchr(__FILE__, '\\') ? std::strrchr(__FILE__, '\\') + 1 : __FILE__)

// 카테고리별 로그 매크로
#if IS_DEBUG_MODE
#define LOG_NORMAL(format, ...) \
    UConsoleManager::Get()->Log(ELogCategory::LogLevel_Normal, __FILENAME__, __LINE__, format, ##__VA_ARGS__)

#define LOG_INFO(format, ...) \
    UConsoleManager::Get()->Log(ELogCategory::LogLevel_Info, __FILENAME__, __LINE__, format, ##__VA_ARGS__)

#define LOG_ERROR(format, ...) \
    UConsoleManager::Get()->Log(ELogCategory::LogLevel_Error, __FILENAME__, __LINE__, format, ##__VA_ARGS__)

#define LOG_WARNING(format, ...) \
    UConsoleManager::Get()->Log(ELogCategory::LogLevel_Warning, __FILENAME__, __LINE__, format, ##__VA_ARGS__)

// 기존 호환성 유지 - 기존 LOG 매크로를 LOG_NORMAL로 연결
#define LOG(format, ...) LOG_NORMAL(format, ##__VA_ARGS__)

// 함수 호출 로그 - LOG_INFO로 변경하여 카테고리 시스템 활용
#define LOG_FUNC_CALL(format, ...) \
    LOG_INFO("[%s:%d] %s: " format, __FILENAME__, __LINE__, __FUNCTION__, ##__VA_ARGS__)

#else
// 릴리즈 모드에서는 모든 로그 제거
#define LOG_NORMAL(format, ...) ((void)0)
#define LOG_INFO(format, ...) ((void)0)
#define LOG_ERROR(format, ...) ((void)0)
#define LOG_WARNING(format, ...) ((void)0)
#define LOG(format, ...) ((void)0)
#define LOG_FUNC_CALL(format, ...) ((void)0)
#endif
#pragma endregion