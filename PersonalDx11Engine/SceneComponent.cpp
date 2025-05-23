#include "SceneComponent.h"
#include "Debug.h"

// 로컬 트랜스폼 변경 시 호출되는 함수
void USceneComponent::OnLocalTransformChanged()
{
    // 1. 나의 월드 트랜스폼 업데이트
    auto Parent = GetSceneParent();
    if (Parent)
    {
        const FTransform& ParentWorldTransform = Parent->GetWorldTransform();
        WorldTransform = LocalToWorld(ParentWorldTransform);
    }
    else
    {
        WorldTransform = LocalTransform;
    }

    // 2. 변경 이벤트 발생
    OnLocalTransformChangedDelegate.Broadcast(LocalTransform);
    OnWorldTransformChangedDelegate.Broadcast(WorldTransform);

    // 3. 자식 컴포넌트들의 월드 트랜스폼 재귀적 업데이트
    PropagateWorldTransformToChildren();
}

// 월드 트랜스폼 변경 시 호출되는 함수
void USceneComponent::OnWorldTransformChanged()
{
    // 1. 나의 로컬 트랜스폼 업데이트
    auto Parent = GetSceneParent();
    if (Parent)
    {
        const FTransform& ParentWorldTransform = Parent->GetWorldTransform();
        LocalTransform = WorldToLocal(ParentWorldTransform);
    }
    else
    {
        LocalTransform = WorldTransform;
    }

    // 2. 변경 이벤트 발생
    OnLocalTransformChangedDelegate.Broadcast(LocalTransform);
    OnWorldTransformChangedDelegate.Broadcast(WorldTransform);

    // 3. 자식 컴포넌트들의 월드 트랜스폼 재귀적 업데이트
    PropagateWorldTransformToChildren();
}

// 부모 변경 시 호출되는 함수
void USceneComponent::OnParentSceneChanged(const std::shared_ptr<USceneComponent>& NewParent)
{
    // 현재 월드 트랜스폼 저장
    FTransform CurrentWorldTransform = GetWorldTransform();

    // 트랜스폼 계층구조
    if (NewParent)
    {
        // 월드 트랜스폼 유지하며 로컬 트랜스폼 재계산
        WorldTransform = CurrentWorldTransform;
        const FTransform& ParentWorldTransform = NewParent->GetWorldTransform();
        LocalTransform = WorldToLocal(ParentWorldTransform);
    }
    else
    {
        // 부모가 없는 경우 로컬=월드
        LocalTransform = CurrentWorldTransform;
        WorldTransform = CurrentWorldTransform;
    }

    // 4. 변경 이벤트 발생
    OnLocalTransformChangedDelegate.Broadcast(LocalTransform);
}

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
        OnLocalTransformChanged();
    }
}

void USceneComponent::SetLocalPosition(const Vector3& InPosition)
{
    if (!FTransform::IsValidPosition(LocalTransform.Position - InPosition))
    {
        return;
    }
    LocalTransform.Position = InPosition;
    OnLocalTransformChanged();
}

void USceneComponent::SetLocalRotation(const Quaternion& InRotation)
{
    if (!FTransform::IsValidRotation((LocalTransform.Rotation, InRotation)))
    {
        return;
    }
    // 정규화 및 저장
    XMVECTOR RotQuat = XMLoadFloat4(&InRotation);
    XMVECTOR ResultQuat = XMQuaternionNormalize(RotQuat);
    XMStoreFloat4(&LocalTransform.Rotation, ResultQuat);

    // 변경 플래그 설정 및 전파
    OnLocalTransformChanged();
}

void USceneComponent::SetLocalRotationEuler(const Vector3& InEuler)
{
    Quaternion InQuat = Math::EulerToQuaternion(InEuler);
    SetLocalRotation(InQuat);
}

void USceneComponent::SetLocalScale(const Vector3& InScale)
{
    if (!FTransform::IsValidScale(LocalTransform.Scale - InScale))
    {
        return;
    }
    LocalTransform.Scale = InScale;
    OnLocalTransformChanged();
}

void USceneComponent::AddLocalPosition(const Vector3& InDeltaPosition)
{
    if (!FTransform::IsValidPosition(InDeltaPosition))
    {
        return;
    }
    Vector3 New = LocalTransform.Position + InDeltaPosition;
    SetLocalPosition(New);
}

void USceneComponent::AddLocalRotation(const Quaternion& InDeltaRotation)
{
    // 변화량이 의미 있는지 확인
    if (!FTransform::IsValidRotation(InDeltaRotation))
    {
        return;
    }

    // 현재 회전에 새 회전을 합성
    XMVECTOR CurrentQuat = XMLoadFloat4(&LocalTransform.Rotation);
    XMVECTOR DeltaQuat = XMLoadFloat4(&InDeltaRotation);

    // 쿼터니언 곱으로 회전 결합 (순서 중요: 델타 * 현재)
    XMVECTOR ResultQuat = XMQuaternionMultiply(DeltaQuat, CurrentQuat);

    // 정규화 및 저장
    ResultQuat = XMQuaternionNormalize(ResultQuat);
    XMStoreFloat4(&LocalTransform.Rotation, ResultQuat);

    // 변경 플래그 설정 및 전파
    OnLocalTransformChanged();
}

void USceneComponent::AddLocalRotationEuler(const Vector3& InDeltaEuler)
{
    Quaternion Delta = Math::EulerToQuaternion(InDeltaEuler);
    AddLocalRotation(Delta);
}

// 월드 트랜스폼 설정자
void USceneComponent::SetWorldTransform(const FTransform& InWorldTransform)
{
    bool bIsUpdate = false;

    bIsUpdate =
        FTransform::IsValidPosition(WorldTransform.Position - InWorldTransform.Position) ||
        FTransform::IsValidScale(WorldTransform.Scale - InWorldTransform.Scale) ||
        FTransform::IsValidRotation(WorldTransform.Rotation, InWorldTransform.Rotation);

    if (bIsUpdate)
    {
        WorldTransform = InWorldTransform;
        OnWorldTransformChanged();
    } 
}

void USceneComponent::SetWorldPosition(const Vector3& InWorldPosition)
{
    if (!FTransform::IsValidPosition(WorldTransform.Position - InWorldPosition))
    {
        return;
    }

    FTransform NewWorldTransform = WorldTransform;
    NewWorldTransform.Position = InWorldPosition;
    SetWorldTransform(NewWorldTransform);
}

void USceneComponent::SetWorldRotation(const Quaternion& InWorldRotation)
{
    if (!FTransform::IsValidRotation(WorldTransform.Rotation,InWorldRotation))
    {
        return;

    }
    FTransform NewWorldTransform = WorldTransform;

    // 정규화 및 저장
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
    {
        return;
    }
    FTransform NewWorldTransform = WorldTransform;
    NewWorldTransform.Scale = InWorldScale;
    SetWorldTransform(NewWorldTransform);
}

void USceneComponent::AddWorldPosition(const Vector3& InDeltaPosition)
{
    Vector3  NewPosition = WorldTransform.Position + InDeltaPosition;
    SetWorldPosition(NewPosition);
}

void USceneComponent::AddWorldRotation(const Quaternion& InDeltaRotation)
{
    if (!FTransform::IsValidRotation(InDeltaRotation))
        return;

    FTransform NewTransform = WorldTransform;

    // 현재 회전에 새 회전을 합성
    XMVECTOR CurrentQuat = XMLoadFloat4(&NewTransform.Rotation);
    XMVECTOR DeltaQuat = XMLoadFloat4(&InDeltaRotation);

    // 쿼터니언 곱으로 회전 결합 (순서 중요: 델타 * 현재)
    XMVECTOR ResultQuat = XMQuaternionMultiply(DeltaQuat, CurrentQuat);

    // 정규화 및 저장
    ResultQuat = XMQuaternionNormalize(ResultQuat);
    XMStoreFloat4(&NewTransform.Rotation, ResultQuat);

    SetWorldTransform(NewTransform);

}

void USceneComponent::AddWorldRotationEuler(const Vector3& InDeltaEuler)
{
    Quaternion InQuat = Math::EulerToQuaternion(InDeltaEuler);
    AddWorldRotation(InQuat);
}

// 트랜스폼 변환 함수
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

FTransform USceneComponent::WorldToLocal(const FTransform& InParentWorldTransform) const
{
    FTransform Result;

    // 스케일: 월드 스케일 / 부모 스케일
    Result.Scale = Vector3(
        WorldTransform.Scale.x / InParentWorldTransform.Scale.x,
        WorldTransform.Scale.y / InParentWorldTransform.Scale.y,
        WorldTransform.Scale.z / InParentWorldTransform.Scale.z
    );

    // 회전: 부모 회전의 역 * 월드 회전
    XMVECTOR ParentRot = XMLoadFloat4(&InParentWorldTransform.Rotation);
    XMVECTOR WorldRot = XMLoadFloat4(&WorldTransform.Rotation);
    XMVECTOR InvParentRot = XMQuaternionInverse(ParentRot);
    XMVECTOR LocalRot = XMQuaternionMultiply(InvParentRot, WorldRot);
    XMStoreFloat4(&Result.Rotation, LocalRot);

    // 위치: (부모 회전의 역 * (월드 위치 - 부모 위치)) / 부모 스케일
    XMVECTOR WorldPos = XMLoadFloat3(&WorldTransform.Position);
    XMVECTOR ParentPos = XMLoadFloat3(&InParentWorldTransform.Position);
    XMVECTOR RelativePos = XMVectorSubtract(WorldPos, ParentPos);

    // 부모 회전의 역 적용
    XMVECTOR UnrotatedPos = XMVector3Rotate(RelativePos, InvParentRot);

    // 부모 스케일의 역 적용
    XMVECTOR InvParentScale = XMVectorReciprocal(XMLoadFloat3(&InParentWorldTransform.Scale));
    XMVECTOR LocalPos = XMVectorMultiply(UnrotatedPos, InvParentScale);

    XMStoreFloat3(&Result.Position, LocalPos);

    return Result;
}

// 트랜스폼 전파 함수
void USceneComponent::PropagateWorldTransformToChildren()
{
    // 자식 컴포넌트에 월드 트랜스폼 변경 전파
    auto Children = GetChildren();
    for (const auto& Child : Children)
    {
        auto SceneChild = Engine::Cast<USceneComponent>(Child.lock());

        if (SceneChild)
        {
            auto NewChildWorldTransform = SceneChild->LocalToWorld(GetWorldTransform());
            SceneChild->SetWorldTransform(NewChildWorldTransform);
        }
    }
}

void USceneComponent::OnParentChanged(const std::shared_ptr<UActorComponent>& NewParent)
{
    UActorComponent::OnParentChanged(NewParent);
    if (auto ParentScene = Engine::Cast<USceneComponent>(NewParent))
    {
        OnParentSceneChanged(ParentScene);
    }
}

const FTransform& USceneComponent::GetWorldTransform() const
{
    return WorldTransform;
}

const FTransform& USceneComponent::GetLocalTransform() const
{
    return LocalTransform;
}

const Vector3 USceneComponent::GetWorldForward() const
{
    Vector3 vLocalForward = Vector3::Forward();
    XMVECTOR LocalFoward = XMVectorSet(vLocalForward.x, vLocalForward.y, vLocalForward.z, 1.0f);

    Matrix RotMatrix = GetWorldTransform().GetRotationMatrix();
    XMVECTOR WorldForward = XMVector3TransformNormal(LocalFoward, RotMatrix);

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
   // 현재 월드 위치 가져오기
    Vector3 WorldPos = GetWorldPosition();

    // 타겟 방향 벡터 계산
    Vector3 Direction = TargetWorldPosition - WorldPos;

    // 방향 벡터가 너무 작으면 회전하지 않음
    if (Direction.LengthSquared() < FTransform::TRANSFORM_EPSILON)
        return;

    // 방향을 정규화
    Direction.Normalize();

    Vector3 Up = Vector3::Up();

    float DotProduct = Vector3::Dot(Up, Direction);
    //Up 과 Dircetion이 펑행
    if ( std::fabs(1 - std::fabs(DotProduct)) < KINDA_SMALL)
    {
        //같은 방향이면 Up = -Forward, 아니면 반대
        Up = DotProduct > 0.0f ? -Vector3::Forward() : Vector3::Forward();
    }

    // 방향에 맞는 회전 쿼터니언 생성
    Quaternion NewRotation = Quaternion::LookRotation(Direction, Up);

    // 월드 회전 설정
    SetWorldRotation(NewRotation);
}

void USceneComponent::RotateAroundAxis(const Vector3& Axis, float AngleDegrees)
{
   // 각도가 너무 작거나 축이 너무 작으면 회전하지 않음
    if (std::abs(AngleDegrees) < FTransform::TRANSFORM_EPSILON || Axis.LengthSquared() < FTransform::TRANSFORM_EPSILON)
        return;

    // 현재 월드 회전 가져오기
    Quaternion WorldRot = GetWorldRotation();

    // 회전축 정규화
    Vector3 NormalizedAxis = Axis;
    NormalizedAxis.Normalize();

    // 라디안으로 변환
    float AngleRadians = Math::DegreeToRad(AngleDegrees);

    // 회전 쿼터니언 생성
    XMVECTOR AxisVec = XMLoadFloat3(&NormalizedAxis);
    XMVECTOR DeltaRot = XMQuaternionRotationAxis(AxisVec, AngleRadians);

    // 현재 회전에 적용
    XMVECTOR CurrentRot = XMLoadFloat4(&WorldRot);
    XMVECTOR ResultRot = XMQuaternionMultiply(DeltaRot, CurrentRot);

    // 정규화 및 적용
    ResultRot = XMQuaternionNormalize(ResultRot);

    Quaternion NewRotation;
    XMStoreFloat4(&NewRotation, ResultRot);

    // 월드 회전 설정
    SetWorldRotation(NewRotation);
}

// 부모 설정 오버라이드
void USceneComponent::SetParent(const std::shared_ptr<USceneComponent>& InParent)
{
    // 부모 변경 전 현재 월드 트랜스폼 저장
    FTransform CurrentWorldTransform = GetWorldTransform();

    // 기존 부모에서 분리 - 새부모설정과정에서 ActorComponent가 처리
    auto OldParent = GetParent();
    //if (OldParent)
    //{
    //    DetachFromParent();
    //}

    // 새 부모가 있으면 연결
    if (InParent)
    {
        // 부모-자식 관계 설정
        UActorComponent::SetParent(InParent);

        // 월드 트랜스폼 유지를 위한 로컬 트랜스폼 계산
        FTransform NewLocal = WorldToLocal(InParent->GetWorldTransform());
        SetLocalTransform(NewLocal);
    }
    else
    {
        // 부모가 없는 경우, 월드원점이 부모
        UActorComponent::SetParent(nullptr);
        LocalTransform = CurrentWorldTransform;
        WorldTransform = CurrentWorldTransform;
    }
}