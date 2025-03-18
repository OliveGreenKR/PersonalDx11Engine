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
    //mutable uint32_t LocalTransformVersion = 0;
    //mutable uint32_t WorldTransformVersion = 0;
    //mutable uint32_t ParentWorldTransformVersion = 0;
    // 추가 필드 및 메서드
    mutable bool bNeedsWorldTransformUpdate = false; // 트랜스폼 업데이트 플래그

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

    //const size_t GetWorldTransformVersion() const { return WorldTransformVersion; }
    //const bool IsWorldTransformDirty() const { return WorldTransformVersion == 0 || bNeedsWorldTransformUpdate; }
    const bool IsWorldTransfromNeedUpdate() const { return bNeedsWorldTransformUpdate; }
    //const bool IsWorldTransformDirty() const { return WorldTransformVersion == 0 || bNeedsWorldTransformUpdate; }


	virtual const char* GetComponentClassName() const override { return "UScene"; }

    // Root 컴포넌트 찾기
    USceneComponent* FindRootSceneComponent() const;

private:
    void MarkLocalTransformDirty();

    void UpdateWorldTransformIfNeeded() const;
    void CalculateWorldTransform() const;
    void OnParentTransformChanged();

    // 서브트리 전체에 업데이트 필요성 전파
    void PropagateUpdateFlagToSubtree() const;

    


private:
    static constexpr float POSITION_THRESHOLD = KINDA_SMALL;
    static constexpr float ROTATION_THRESHOLD = KINDA_SMALL;
    static constexpr float SCALE_THRESHOLD = KINDA_SMALL;
};
