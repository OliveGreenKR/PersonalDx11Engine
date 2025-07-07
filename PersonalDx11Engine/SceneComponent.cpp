// SceneComponent.cpp 간단한 수정

#include "SceneComponent.h"
#include "Debug.h"

#pragma region Core Transform Functions

// 사례 2: 나의 로컬 트랜스폼 변경
void USceneComponent::OnLocalTransformChanged()
{
    // 1. 나의 월드 트랜스폼 업데이트
    auto Parent = GetSceneParent();
    if (Parent)
    {
        WorldTransform = LocalToWorld(Parent->GetWorldTransform());
    }
    else
    {
        WorldTransform = LocalTransform;
    }

    // 2. 이벤트 발생
    OnLocalTransformChangedDelegate.Broadcast(LocalTransform);
    OnWorldTransformChangedDelegate.Broadcast(WorldTransform);

    // 3. 자식들의 월드 업데이트 (사례 1 적용)
    PropagateWorldTransformToChildren();
}

// 사례 3: 나의 월드 트랜스폼 변경 (외부에서 직접 설정)
void USceneComponent::OnWorldTransformChanged()
{
    // 1. 나의 로컬 트랜스폼 재계산
    auto Parent = GetSceneParent();
    if (Parent)
    {
        LocalTransform = WorldToLocal(WorldTransform, Parent->GetWorldTransform());
    }
    else
    {
        LocalTransform = WorldTransform;
    }

    // 2. 이벤트 발생
    OnLocalTransformChangedDelegate.Broadcast(LocalTransform);
    OnWorldTransformChangedDelegate.Broadcast(WorldTransform);

    // 3. 자식들의 월드 업데이트 (사례 1 적용)
    PropagateWorldTransformToChildren();
}

// 사례 1: 부모의 월드 트랜스폼 변경 → 자식들에게 전파
void USceneComponent::PropagateWorldTransformToChildren()
{
    auto Children = GetChildren();
    for (const auto& Child : Children)
    {
        auto SceneChild = Engine::Cast<USceneComponent>(Child.lock());
        if (SceneChild)
        {
            // 자식의 로컬 유지하면서 월드만 재계산
            FTransform NewChildWorld = SceneChild->LocalToWorld(GetWorldTransform());

            // 직접 업데이트 (로컬 재계산 없이)
            SceneChild->WorldTransform = NewChildWorld;

            // 이벤트 발생
            SceneChild->OnWorldTransformChangedDelegate.Broadcast(SceneChild->WorldTransform);

            // 재귀적으로 자식의 자식들도 처리
            SceneChild->PropagateWorldTransformToChildren();
        }
    }
}

// 사례 4: 부모 변경
void USceneComponent::OnParentSceneChanged(const std::shared_ptr<USceneComponent>& NewParent)
{
    // 현재 월드 트랜스폼 보존
    FTransform CurrentWorldTransform = GetWorldTransform();

    if (NewParent)
    {
        // 새 부모 기준으로 로컬 재계산 (월드는 유지)
        LocalTransform = WorldToLocal(CurrentWorldTransform, NewParent->GetWorldTransform());
        WorldTransform = CurrentWorldTransform; // 월드는 변화없음
    }
    else
    {
        // 루트가 되면 로컬 = 월드
        LocalTransform = CurrentWorldTransform;
        WorldTransform = CurrentWorldTransform;
    }

    // 이벤트 발생 (월드는 변화없으므로 자식들에게 전파 안함)
    OnLocalTransformChangedDelegate.Broadcast(LocalTransform);
}

#pragma endregion

#pragma region Transform Conversion Functions

FTransform USceneComponent::LocalToWorld(const FTransform& InParentWorldTransform) const
{
    FTransform Result;

    // 스케일: 부모 스케일 * 로컬 스케일
    Result.Scale = Vector3(
        InParentWorldTransform.Scale.x * LocalTransform.Scale.x,
        InParentWorldTransform.Scale.y * LocalTransform.Scale.y,
        InParentWorldTransform.Scale.z * LocalTransform.Scale.z
    );

    // 회전: 부모 회전 * 로컬 회전
    XMVECTOR ParentRot = XMLoadFloat4(&InParentWorldTransform.Rotation);
    XMVECTOR LocalRot = XMLoadFloat4(&LocalTransform.Rotation);
    XMVECTOR WorldRot = XMQuaternionMultiply(ParentRot, LocalRot);
    XMStoreFloat4(&Result.Rotation, WorldRot);

    // 위치: 부모 위치 + (부모 회전 * 부모 스케일 * 로컬 위치)
    XMVECTOR LocalPos = XMLoadFloat3(&LocalTransform.Position);
    XMVECTOR ParentScale = XMLoadFloat3(&InParentWorldTransform.Scale);

    // 스케일 적용
    XMVECTOR ScaledPos = XMVectorMultiply(LocalPos, ParentScale);

    // 회전 적용
    XMVECTOR RotatedPos = XMVector3Rotate(ScaledPos, ParentRot);

    // 부모 위치에 더하기
    XMVECTOR ParentPos = XMLoadFloat3(&InParentWorldTransform.Position);
    XMVECTOR WorldPos = XMVectorAdd(ParentPos, RotatedPos);

    XMStoreFloat3(&Result.Position, WorldPos);

    return Result;
}


// 특정 월드 트랜스폼을 부모 기준 로컬로 변환
FTransform USceneComponent::WorldToLocal(const FTransform& InTargetWorldTransform, const FTransform& InParentWorldTransform) const
{
    FTransform Result;

    // 스케일: 타겟 월드 스케일 / 부모 스케일
    Result.Scale = Vector3(
        InTargetWorldTransform.Scale.x / InParentWorldTransform.Scale.x,
        InTargetWorldTransform.Scale.y / InParentWorldTransform.Scale.y,
        InTargetWorldTransform.Scale.z / InParentWorldTransform.Scale.z
    );

    // 회전: 부모 회전의 역 * 타겟 월드 회전
    XMVECTOR ParentRot = XMLoadFloat4(&InParentWorldTransform.Rotation);
    XMVECTOR TargetWorldRot = XMLoadFloat4(&InTargetWorldTransform.Rotation);
    XMVECTOR InvParentRot = XMQuaternionInverse(ParentRot);
    XMVECTOR LocalRot = XMQuaternionMultiply(InvParentRot, TargetWorldRot);
    XMStoreFloat4(&Result.Rotation, LocalRot);

    // 위치: (부모 회전의 역 * (타겟 월드 위치 - 부모 위치)) / 부모 스케일
    XMVECTOR TargetWorldPos = XMLoadFloat3(&InTargetWorldTransform.Position);
    XMVECTOR ParentPos = XMLoadFloat3(&InParentWorldTransform.Position);
    XMVECTOR RelativePos = XMVectorSubtract(TargetWorldPos, ParentPos);

    // 부모 회전의 역 적용
    XMVECTOR UnrotatedPos = XMVector3Rotate(RelativePos, InvParentRot);

    // 부모 스케일의 역 적용 (정확한 나누기)
    XMVECTOR ParentScale = XMLoadFloat3(&InParentWorldTransform.Scale);
    XMVECTOR LocalPos = XMVectorDivide(UnrotatedPos, ParentScale);

    XMStoreFloat3(&Result.Position, LocalPos);

    return Result;
}

#pragma endregion

#pragma region Transform Accessors

const FTransform& USceneComponent::GetWorldTransform() const
{
    return WorldTransform;
}

const FTransform& USceneComponent::GetLocalTransform() const
{
    return LocalTransform;
}

#pragma endregion

#pragma region Utility Functions

const Vector3 USceneComponent::GetWorldForward() const
{
    Vector3 vLocalForward = Vector3::Forward();
    XMVECTOR LocalForward = XMVectorSet(vLocalForward.x, vLocalForward.y, vLocalForward.z, 1.0f);

    Matrix RotMatrix = GetWorldTransform().GetRotationMatrix();
    XMVECTOR WorldForward = XMVector3TransformNormal(LocalForward, RotMatrix);

    Vector3 Result;
    XMStoreFloat3(&Result, WorldForward);
    return Result;
}

const Vector3 USceneComponent::GetWorldUp() const
{
    Vector3 vLocalUp = Vector3::Up();
    XMVECTOR LocalUp = XMVectorSet(vLocalUp.x, vLocalUp.y, vLocalUp.z, 1.0f);

    Matrix RotMatrix = GetWorldTransform().GetRotationMatrix();
    XMVECTOR WorldUp = XMVector3TransformNormal(LocalUp, RotMatrix);

    Vector3 Result;
    XMStoreFloat3(&Result, WorldUp);
    return Result;
}

const Vector3 USceneComponent::GetWorldRight() const
{
    Vector3 vLocalRight = Vector3::Right();
    XMVECTOR LocalRight = XMVectorSet(vLocalRight.x, vLocalRight.y, vLocalRight.z, 1.0f);

    Matrix RotMatrix = GetWorldTransform().GetRotationMatrix();
    XMVECTOR WorldRight = XMVector3TransformNormal(LocalRight, RotMatrix);

    Vector3 Result;
    XMStoreFloat3(&Result, WorldRight);
    return Result;
}

void USceneComponent::LookAt(const Vector3& TargetWorldPosition)
{
    Vector3 WorldPos = GetWorldPosition();
    Vector3 Direction = TargetWorldPosition - WorldPos;

    if (Direction.LengthSquared() < FTransform::TRANSFORM_EPSILON)
        return;

    Direction.Normalize();

    Vector3 Up = Vector3::Up();
    float DotProduct = Vector3::Dot(Up, Direction);

    if (std::fabs(1 - std::fabs(DotProduct)) < KINDA_SMALL)
    {
        Up = DotProduct > 0.0f ? -Vector3::Forward() : Vector3::Forward();
    }

    Quaternion NewRotation = Quaternion::LookRotation(Direction, Up);
    SetWorldRotation(NewRotation);
}

void USceneComponent::RotateAroundAxis(const Vector3& Axis, float AngleDegrees)
{
    if (std::abs(AngleDegrees) < FTransform::TRANSFORM_EPSILON || Axis.LengthSquared() < FTransform::TRANSFORM_EPSILON)
        return;

    Quaternion WorldRot = GetWorldRotation();
    Vector3 NormalizedAxis = Axis;
    NormalizedAxis.Normalize();

    float AngleRadians = Math::DegreeToRad(AngleDegrees);

    XMVECTOR AxisVec = XMLoadFloat3(&NormalizedAxis);
    XMVECTOR DeltaRot = XMQuaternionRotationAxis(AxisVec, AngleRadians);
    XMVECTOR CurrentRot = XMLoadFloat4(&WorldRot);
    XMVECTOR ResultRot = XMQuaternionMultiply(DeltaRot, CurrentRot);
    ResultRot = XMQuaternionNormalize(ResultRot);

    Quaternion NewRotation;
    XMStoreFloat4(&NewRotation, ResultRot);
    SetWorldRotation(NewRotation);
}

#pragma endregion

#pragma region Hierarchy Management

void USceneComponent::OnParentChanged(const std::shared_ptr<UActorComponent>& NewParent)
{
    UActorComponent::OnParentChanged(NewParent);
    if (auto ParentScene = Engine::Cast<USceneComponent>(NewParent))
    {
        OnParentSceneChanged(ParentScene);
    }
}

void USceneComponent::SetParent(const std::shared_ptr<USceneComponent>& InParent)
{
    FTransform CurrentWorldTransform = GetWorldTransform();

    // 부모 변경 후 월드 좌표 보존
    UActorComponent::SetParent(InParent);

    if (InParent)
    {
        LocalTransform = WorldToLocal(CurrentWorldTransform, InParent->GetWorldTransform());
        WorldTransform = CurrentWorldTransform;
    }
    else
    {
        LocalTransform = CurrentWorldTransform;
        WorldTransform = CurrentWorldTransform;
    }

    // 로컬만 변경됨 (월드는 보존되므로 자식 전파 불필요)
    OnLocalTransformChangedDelegate.Broadcast(LocalTransform);
}

#pragma endregion

#pragma region Local Transform Setters

void USceneComponent::SetLocalTransform(const FTransform& InTransform)
{
    bool bChanged = false;

    if (FTransform::IsValidPosition(LocalTransform.Position - InTransform.Position))
    {
        LocalTransform.Position = InTransform.Position;
        bChanged = true;
    }

    if (FTransform::IsValidRotation(LocalTransform.Rotation, InTransform.Rotation))
    {
        LocalTransform.Rotation = InTransform.Rotation;
        bChanged = true;
    }

    if (FTransform::IsValidScale(LocalTransform.Scale - InTransform.Scale))
    {
        LocalTransform.Scale = InTransform.Scale;
        bChanged = true;
    }

    if (bChanged)
    {
        OnLocalTransformChanged(); // 사례 2
    }
}

void USceneComponent::SetLocalPosition(const Vector3& InPosition)
{
    if (!FTransform::IsValidPosition(LocalTransform.Position - InPosition))
        return;

    LocalTransform.Position = InPosition;
    OnLocalTransformChanged(); // 사례 2
}

void USceneComponent::SetLocalRotation(const Quaternion& InRotation)
{
    if (!FTransform::IsValidRotation(LocalTransform.Rotation, InRotation))
        return;

    XMVECTOR RotQuat = XMLoadFloat4(&InRotation);
    XMVECTOR ResultQuat = XMQuaternionNormalize(RotQuat);
    XMStoreFloat4(&LocalTransform.Rotation, ResultQuat);

    OnLocalTransformChanged(); // 사례 2
}

void USceneComponent::SetLocalRotationEuler(const Vector3& InEuler)
{
    Quaternion InQuat = Math::EulerToQuaternion(InEuler);
    SetLocalRotation(InQuat);
}

void USceneComponent::SetLocalScale(const Vector3& InScale)
{
    if (!FTransform::IsValidScale(LocalTransform.Scale - InScale))
        return;

    LocalTransform.Scale = InScale;
    OnLocalTransformChanged(); // 사례 2
}

void USceneComponent::AddLocalPosition(const Vector3& InDeltaPosition)
{
    if (!FTransform::IsValidPosition(InDeltaPosition))
        return;

    Vector3 New = LocalTransform.Position + InDeltaPosition;
    SetLocalPosition(New);
}

void USceneComponent::AddLocalRotation(const Quaternion& InDeltaRotation)
{
    if (!FTransform::IsValidRotation(InDeltaRotation))
        return;

    XMVECTOR CurrentQuat = XMLoadFloat4(&LocalTransform.Rotation);
    XMVECTOR DeltaQuat = XMLoadFloat4(&InDeltaRotation);
    XMVECTOR ResultQuat = XMQuaternionMultiply(DeltaQuat, CurrentQuat);
    ResultQuat = XMQuaternionNormalize(ResultQuat);
    XMStoreFloat4(&LocalTransform.Rotation, ResultQuat);

    OnLocalTransformChanged(); // 사례 2
}

void USceneComponent::AddLocalRotationEuler(const Vector3& InDeltaEuler)
{
    Quaternion Delta = Math::EulerToQuaternion(InDeltaEuler);
    AddLocalRotation(Delta);
}

#pragma endregion

#pragma region World Transform Setters

void USceneComponent::SetWorldTransform(const FTransform& InWorldTransform)
{
    bool bIsUpdate =
        FTransform::IsValidPosition(WorldTransform.Position - InWorldTransform.Position) ||
        FTransform::IsValidScale(WorldTransform.Scale - InWorldTransform.Scale) ||
        FTransform::IsValidRotation(WorldTransform.Rotation, InWorldTransform.Rotation);

    if (bIsUpdate)
    {
        WorldTransform = InWorldTransform;
        OnWorldTransformChanged(); // 사례 3
    }
}

void USceneComponent::SetWorldPosition(const Vector3& InWorldPosition)
{
    if (!FTransform::IsValidPosition(WorldTransform.Position - InWorldPosition))
        return;

    FTransform NewWorldTransform = WorldTransform;
    NewWorldTransform.Position = InWorldPosition;
    SetWorldTransform(NewWorldTransform);
}

void USceneComponent::SetWorldRotation(const Quaternion& InWorldRotation)
{
    if (!FTransform::IsValidRotation(WorldTransform.Rotation, InWorldRotation))
        return;

    FTransform NewWorldTransform = WorldTransform;
    XMVECTOR RotQuat = XMLoadFloat4(&InWorldRotation);
    XMVECTOR ResultQuat = XMQuaternionNormalize(RotQuat);
    XMStoreFloat4(&NewWorldTransform.Rotation, ResultQuat);
    SetWorldTransform(NewWorldTransform);
}

void USceneComponent::SetWorldRotationEuler(const Vector3& InWorldEuler)
{
    Quaternion InQuat = Math::EulerToQuaternion(InWorldEuler);
    SetWorldRotation(InQuat);
}

void USceneComponent::SetWorldScale(const Vector3& InWorldScale)
{
    if (!FTransform::IsValidScale(WorldTransform.Scale - InWorldScale))
        return;

    FTransform NewWorldTransform = WorldTransform;
    NewWorldTransform.Scale = InWorldScale;
    SetWorldTransform(NewWorldTransform);
}

void USceneComponent::AddWorldPosition(const Vector3& InDeltaPosition)
{
    Vector3 NewPosition = WorldTransform.Position + InDeltaPosition;
    SetWorldPosition(NewPosition);
}

void USceneComponent::AddWorldRotation(const Quaternion& InDeltaRotation)
{
    if (!FTransform::IsValidRotation(InDeltaRotation))
        return;

    FTransform NewTransform = WorldTransform;
    XMVECTOR CurrentQuat = XMLoadFloat4(&NewTransform.Rotation);
    XMVECTOR DeltaQuat = XMLoadFloat4(&InDeltaRotation);
    XMVECTOR ResultQuat = XMQuaternionMultiply(DeltaQuat, CurrentQuat);
    ResultQuat = XMQuaternionNormalize(ResultQuat);
    XMStoreFloat4(&NewTransform.Rotation, ResultQuat);

    SetWorldTransform(NewTransform);
}

void USceneComponent::AddWorldRotationEuler(const Vector3& InDeltaEuler)
{
    Quaternion InQuat = Math::EulerToQuaternion(InDeltaEuler);
    AddWorldRotation(InQuat);
}

#pragma endregion