#pragma once
#include "ActorComponent.h"
#include "Transform.h"
#include "Delegate.h"
#include "Math.h"

class USceneComponent : public UActorComponent
{
public:
    FDelegate<const FTransform&> OnTransformChangedDelegate;
private:
    FTransform LocalTransform;
    mutable FTransform CachedWorldTransform;
    mutable uint32_t LocalTransformVersion = 0;
    mutable uint32_t WorldTransformVersion = 0;
    mutable uint32_t ParentWorldTransformVersion = 0;
public:
    const std::shared_ptr<USceneComponent>& GetSceneParent() const {
        return std::dynamic_pointer_cast<USceneComponent>(GetParent());
    }
    // Setter
    void SetLocalTransform(const FTransform& InTransform);
    void SetLocalPosition(const Vector3& InPosition);
    void SetLocalRotation(const Quaternion& InRotation);
    void SetLocalRotationEuler(const Vector3& InEuler);
    void SetLocalScale(const Vector3& InScale);

    void AddLocalPosition(const Vector3& InPosition);
    void AddLocalRotationEuler(const Vector3& InDeltaEuler);
    void AddLocalRotation(const Quaternion& InRotation);

    void SetWorldPosition(const Vector3& WorldPosition);

    void RotateLocalAroundAxis(const Vector3& InAxis, float AngleDegrees);

    void LookAt(const Vector3& WorldTargetPos);

    // 트랜스폼 접근자
    const FTransform& GetLocalTransform() const { return LocalTransform; }
    const FTransform& GetWorldTransform() const;

    //Getter
    const Vector3& GetLocalPosition() const;
    const Quaternion& GetLocalRotation() const;
    const Vector3& GetLocalScale() const;

    Vector3 GetWorldPosition() const;
    Quaternion GetWorldRotation() const;
    Vector3 GetWorldScale() const; 

    const size_t GetWorldTransformVersion() const { return WorldTransformVersion; }
    const bool IsWorldTransformDirty() const { return WorldTransformVersion == 0;  }


	virtual const char* GetComponentClassName() const override { return "UScene"; }



private:

    void MarkLocalTransformDirty();

    void UpdateWorldTransformIfNeeded() const;
    void NotifyChildrenOfTransformChange();

    void CalculateWorldTransform() const;
    void OnParentTransformChanged();

private:
    static constexpr float POSITION_THRESHOLD = KINDA_SMALL;
    static constexpr float ROTATION_THRESHOLD = KINDA_SMALL;
    static constexpr float SCALE_THRESHOLD = KINDA_SMALL;
};
