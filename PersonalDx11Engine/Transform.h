#pragma once
#include "Math.h"
#include "Delegate.h"

using namespace DirectX;

struct FTransform
{
public:
	FTransform() = default;

	// ���� ������
	FTransform(const FTransform& Other) :
		Position(Other.Position),
		Rotation(Other.Rotation),
		Scale(Other.Scale),
		TransformVersion(Other.TransformVersion)
	{
		// ��������Ʈ�� �������� ���� - ���ο� ��ü�� �ڽŸ��� �̺�Ʈ �����ʸ� ������ ��
	}

	// �̵� ������
	FTransform(FTransform&& Other) noexcept :
		Position(std::move(Other.Position)),
		Rotation(std::move(Other.Rotation)),
		Scale(std::move(Other.Scale)),
		TransformVersion(Other.TransformVersion),
		OnTransformChangedDelegate(std::move(Other.OnTransformChangedDelegate))
	{
	}

	// ���� �Ҵ� ������
	FTransform& operator=(const FTransform& Other)
	{
		if (this != &Other)
		{
			Position = Other.Position;
			Rotation = Other.Rotation;
			Scale = Other.Scale;
			TransformVersion = Other.TransformVersion;

			// ��������Ʈ�� �������� �ʰ� ����
			// ���� �����ʵ鿡�� ���� �˸�
			NotifyTransformChanged();
		}
		return *this;
	}

	// �̵� �Ҵ� ������
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
	// ���� ����ȭ�� ���� ��ȭ ���� �Ӱ谪
	static constexpr float PositionThreshold = KINDA_SMALL;
	static constexpr float RotationThreshold = KINDA_SMALL;
	static constexpr float ScaleThreshold = KINDA_SMALL;
};