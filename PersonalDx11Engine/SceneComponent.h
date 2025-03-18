﻿#pragma once
#include "ActorComponent.h"
#include "Transform.h"
#include "Delegate.h"
#include "Math.h"

class USceneComponent : public UActorComponent
{
public:
    // 트랜스폼 변경 이벤트 델리게이트
    FDelegate<const FTransform&> OnLocalTransformChangedDelegate;
    FDelegate<const FTransform&> OnWorldTransformChangedDelegate;

private:
    FTransform LocalTransform;                      // 로컬 트랜스폼 (부모 기준)
    mutable FTransform WorldTransform;              // 캐싱된 월드 트랜스폼
    mutable bool bLocalTransformDirty = false;              // 로컬 트랜스폼 변경 플래그
    mutable bool bWorldTransformDirty = false;              // 월드 트랜스폼 변경 플래그

public:
    const bool IsLocalDirty() const { return bLocalTransformDirty; }
    const bool IsWorldDirty() const { return bWorldTransformDirty; }

public:
    // 부모 접근자
    std::shared_ptr<USceneComponent> GetSceneParent() const {
        return std::dynamic_pointer_cast<USceneComponent>(GetParent());
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
	const Vector3& GetLocalPosition() const { return GetLocalTransform().Position; }
	const Quaternion& GetLocalRotation() const { return GetLocalTransform().Rotation; }
	const Vector3& GetLocalScale() const { return GetLocalTransform().Scale; }

    Vector3 GetWorldPosition() { return GetWorldTransform().Position; }
    Quaternion GetWorldRotation() { return GetWorldTransform().Rotation; }
    Vector3 GetWorldScale() { return GetWorldTransform().Scale; }

    // 특수 기능
    void LookAt(const Vector3& TargetWorldPosition);
    void RotateAroundAxis(const Vector3& Axis, float AngleDegrees);

    // 유틸리티 함수
    virtual const char* GetComponentClassName() const override { return "UScene"; }

    // 부모 설정 오버라이드
    void SetParent(const std::shared_ptr<USceneComponent>& InParent);

    //자식의 월드 좌표변환
    static FTransform LocalToWorld(const FTransform& ChildLocal, const FTransform& ParentWorld);
    //자식의 로컬 좌표 변환
    static FTransform WorldToLocal(const FTransform& ChildWorld, const FTransform& ParentWorld);

protected:
    // 트랜스폼 변환 및 업데이트 함수
    void UpdateWorldTransform() const;
    void UpdateLocalTransform();
    void PropagateWorldTransformToChildren();

    //자식의 로컬 -> 월드 좌표변환
    FTransform LocalToWorld(const FTransform& InParentWorldTransform) const;
    //자식의 월드 -> 로컬 
    FTransform WorldToLocal(const FTransform& InParentWorldTransform) const;

    // 이벤트 설정 함수
    void MarkLocalTransformDirty();
    void MarkWorldTransformDirty();

private:
    static constexpr float TRANSFORM_EPSILON = KINDA_SMALL;
};