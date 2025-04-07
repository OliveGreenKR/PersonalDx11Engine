#pragma once
#include "ResourceHandle.h"
#include "define.h"
#include "Math.h"
#include <vector>

class DebugDrawerManager
{
	enum class DebugDrawType
	{
		Sphere,
		Box,
		Line, 
		Arrow,
	};

	// 디버그 도형 기본 인터페이스
	class IDebugDrawable
	{
	public:
		virtual ~IDebugDrawable() = default;
		virtual bool ShouldRender() const = 0;
		virtual void Update(float DeltaTime) = 0;
		virtual bool FillRenderData(class UCamera* Camera, class IRenderData* OutRenderData) const = 0;
	};

private:
	DebugDrawerManager();
	~DebugDrawerManager() = default;

	DebugDrawerManager(const DebugDrawerManager&) = delete;
	DebugDrawerManager& operator=(const DebugDrawerManager&) = delete;
	DebugDrawerManager(DebugDrawerManager&&) = delete;
	DebugDrawerManager& operator=(const DebugDrawerManager&&) = delete;
public:
	void Tick(const float DeltaTime);

	// 단순 도형 API (최적화된 직접 렌더링)
	void DrawLine(const Vector3& Start, const Vector3& End, const Vector4& Color,
				  float Thickness = 1.0f,
				  float DurationTime = 0.0f);

	void DrawSphere(const Vector3& Center, const float Radius, const Vector4& Color,
			,
					float DurationTime = 0.0f);

	void DrawBox(const Vector3& Center, const Vector3& HalfExtents, const Quaternion& Rotation,
				 const Vector4& Color,
				 float DurationTime = 0.0f);

private:
	FResourceHandle Sphere_High_Handle;
	FResourceHandle Sphere_Mid_Handle;
	FResourceHandle Sphere_Low_Handle;
	FResourceHandle BoxHandle;

	//std::vector<DebubElement> DrawElements;
};