#pragma once
#include "Math.h"
#include "Delegate.h"

using namespace DirectX;

struct FTransform
{
public:
	FTransform() = default;

	// 복사 생성자
	FTransform(const FTransform& Other) :
		Position(Other.Position),
		Rotation(Other.Rotation),
		Scale(Other.Scale),
		TransformVersion(Other.TransformVersion)
	{
		// 델리게이트는 복사하지 않음 - 새로운 객체는 자신만의 이벤트 리스너를 가져야 함
	}

	// 이동 생성자
	FTransform(FTransform&& Other) noexcept :
		Position(std::move(Other.Position)),
		Rotation(std::move(Other.Rotation)),
		Scale(std::move(Other.Scale)),
		TransformVersion(Other.TransformVersion),
		OnTransformChangedDelegate(std::move(Other.OnTransformChangedDelegate))
	{
	}

	// 복사 할당 연산자
	FTransform& operator=(const FTransform& Other)
	{
		if (this != &Other)
		{
			Position = Other.Position;
			Rotation = Other.Rotation;
			Scale = Other.Scale;
			TransformVersion = Other.TransformVersion;

			// 델리게이트는 복사하지 않고 유지
			// 기존 리스너들에게 변경 알림
			NotifyTransformChanged();
		}
		return *this;
	}

	// 이동 할당 연산자
	FTransform& operator=(FTransform&& Other) noexcept
	{
		if (this != &Other)
		{
			Position = std::move(Other.Position);
			Rotation = std::move(Other.Rotation);
			Scale = std::move(Other.Scale);
			TransformVersion = Other.TransformVersion;
			OnTransformChangedDelegate = std::move(Other.OnTransformChangedDelegate);
		}
		return *this;
	}

	//{ Pitch, Yaw, Roll }
	void SetEulerRotation(const Vector3& InEulerAngles);
	void SetRotation(const Quaternion& InQuaternion);
	void SetPosition(const Vector3& InPosition);
	void SetScale(const Vector3& InScale);

	void AddPosition(const Vector3& InPosition);
	void AddEulerRotation(const Vector3& InEulerAngles);
	void AddRotation(const Quaternion& InQuaternion);
	void RotateAroundAxis(const Vector3& InAxis, float AngleDegrees);

	const Vector3& GetPosition() const { return Position; }
	const Vector3& GetScale() const { return Scale; }
	const Vector3 GetEulerRotation() const { return Math::QuaternionToEuler(Rotation);}
	const Quaternion& GetRotation() const { return Rotation; }
	const uint32_t GetVersion() const { return TransformVersion; }

	Matrix GetTranslationMatrix() const;
	Matrix GetScaleMatrix() const;
	Matrix GetRotationMatrix() const;
	Matrix GetModelingMatrix() const;

	static FTransform  InterpolateTransform(const FTransform& Start, const FTransform& End, float Alpha);
public:
	//never broadcast directly
	FDelegate<const FTransform&> OnTransformChangedDelegate;

private:
	void NotifyTransformChanged();
private:
	Vector3 Position = Vector3(0.0f, 0.0f, 0.0f);
	Quaternion Rotation = Quaternion(0.0f, 0.0f, 0.0f, 1.0f);
	Vector3 Scale = Vector3(1.0f, 1.0f, 1.0f);


private:
	uint32_t TransformVersion = 0;
	// 성능 최적화를 위한 변화 감지 임계값
	static constexpr float PositionThreshold = KINDA_SMALL;
	static constexpr float RotationThreshold = KINDA_SMALL;
	static constexpr float ScaleThreshold = KINDA_SMALL;
};