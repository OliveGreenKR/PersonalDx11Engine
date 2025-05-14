#pragma once
#include "ActorComponent.h"
#include "Transform.h"
#include "Delegate.h"
#include "Math.h"
#include "TypeCast.h"

class USceneComponent : public UActorComponent
{
public:
    // 트랜스폼 변경 이벤트 델리게이트
    TDelegate<const FTransform&> OnLocalTransformChangedDelegate;
    TDelegate<const FTransform&> OnWorldTransformChangedDelegate;

private:
    FTransform LocalTransform;                      // 로컬 트랜스폼 (부모 기준)
    mutable FTransform WorldTransform;              // 캐싱된 월드 트랜스폼

public:
    virtual ~USceneComponent() = default;

    // 부모 접근자
    std::shared_ptr<USceneComponent> GetSceneParent() const {
        return Engine::Cast<USceneComponent>(GetParent());
    }

    // 로컬 트랜스폼 설정자
    void SetLocalTransform(const FTransform& InTransform);
    void SetLocalPosition(const Vector3& InPosition);
    void SetLocalRotation(const Quaternion& InRotation);
    void SetLocalRotationEuler(const Vector3& InEuler);
    void SetLocalScale(const Vector3& InScale);   

    // 로컬 트랜스폼 추가자
    void AddLocalPosition(const Vector3& InDeltaPosition);
    void AddLocalRotation(const Quaternion& InDeltaRotation);
    void AddLocalRotationEuler(const Vector3& InDeltaEuler);

    // 월드 트랜스폼 설정자
    void SetWorldTransform(const FTransform& InWorldTransform);
    void SetWorldPosition(const Vector3& InWorldPosition);
    void SetWorldRotation(const Quaternion& InWorldRotation);
    void SetWorldRotationEuler(const Vector3& InWorldEuler);
    void SetWorldScale(const Vector3& InWorldScale);

    // 월드 트랜스폼 추가자
    void AddWorldPosition(const Vector3& InDeltaPosition);
    void AddWorldRotation(const Quaternion& InDeltaRotation);
    void AddWorldRotationEuler(const Vector3& InDeltaEuler);

    // 트랜스폼 접근자
    const FTransform& GetLocalTransform() const;
    const FTransform& GetWorldTransform() const;

    // 위치, 회전, 크기 개별 접근자
	Vector3 GetLocalPosition() const { return GetLocalTransform().Position; }
	Quaternion GetLocalRotation() const { return GetLocalTransform().Rotation; }
	Vector3 GetLocalScale() const { return GetLocalTransform().Scale; }

    Vector3 GetWorldPosition() const { return GetWorldTransform().Position; }
    Quaternion GetWorldRotation() const { return GetWorldTransform().Rotation; }
    Vector3 GetWorldScale() const { return GetWorldTransform().Scale; }

    const Vector3 GetWorldForward() const;
    const Vector3 GetWorldUp() const;
    const Vector3 GetWorldRight() const;

    // 특수 기능
    void LookAt(const Vector3& TargetWorldPosition);
    void RotateAroundAxis(const Vector3& Axis, float AngleDegrees);

    // 유틸리티 함수
    virtual const char* GetComponentClassName() const override { return "UScene"; }

    // 부모 설정 오버라이드
    void SetParent(const std::shared_ptr<USceneComponent>& InParent);

    //자식의 로컬 -> 월드 좌표변환
    FTransform LocalToWorld(const FTransform& InParentWorldTransform) const;
    //자식의 월드 -> 로컬 
    FTransform WorldToLocal(const FTransform& InParentWorldTransform) const;

protected:

    void OnLocalTransformChanged();
    void OnWorldTransformChanged();
    void OnParentSceneChanged(const std::shared_ptr<USceneComponent>& NewParent);

    //월드 트랜스폼 업데이트 전파
    void PropagateWorldTransformToChildren();


private:
    void OnParentChanged(const std::shared_ptr<UActorComponent>& NewParent) override;

    static constexpr float TRANSFORM_EPSILON = KINDA_SMALLER;
};
