#include "SceneComponent.h"


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
void USceneComponent::OnChangeParent(const std::shared_ptr<USceneComponent>& NewParent)
{
    // 현재 월드 트랜스폼 저장
    FTransform CurrentWorldTransform = GetWorldTransform();

    // 1. ActorComponent 부모-자식 관계 설정
    if (auto OldParent = GetParent())
    {
        UActorComponent::SetParent(nullptr);
    }

    // 2. 새 부모와의 관계 설정
    if (NewParent)
    {
        UActorComponent::SetParent(NewParent);

        // 3. 월드 트랜스폼 유지하며 로컬 트랜스폼 재계산
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

    if ((LocalTransform.Position - InTransform.Position).LengthSquared() > TRANSFORM_EPSILON)
    {
        LocalTransform.Position = InTransform.Position;
        bChanged = true;
    }

    float RotDot = Quaternion::Dot(LocalTransform.Rotation, InTransform.Rotation);
    if (std::abs(1.0f - std::abs(RotDot)) > TRANSFORM_EPSILON)
    {
        LocalTransform.Rotation = InTransform.Rotation;
        bChanged = true;
    }

    if ((LocalTransform.Scale - InTransform.Scale).LengthSquared() > TRANSFORM_EPSILON)
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
    if ((LocalTransform.Position - InPosition).LengthSquared() > TRANSFORM_EPSILON)
    {
        LocalTransform.Position = InPosition;
        OnLocalTransformChanged();
    }
}

void USceneComponent::SetLocalRotation(const Quaternion& InRotation)
{
     // 변화량이 의미 있는지 확인
    float Dot = Quaternion::Dot(Quaternion::Identity, InRotation);
    float ChangeMagnitude = std::abs(1.0f - std::abs(Dot));

    if (ChangeMagnitude > TRANSFORM_EPSILON)
    {
         // 정규화 및 저장
        XMVECTOR RotQuat = XMLoadFloat4(&InRotation);
        XMVECTOR ResultQuat = XMQuaternionNormalize(RotQuat);
        XMStoreFloat4(&LocalTransform.Rotation, ResultQuat);

        // 변경 플래그 설정 및 전파
        OnLocalTransformChanged();
    }
}

void USceneComponent::SetLocalRotationEuler(const Vector3& InEuler)
{
    Quaternion InQuat = Math::EulerToQuaternion(InEuler);
    SetLocalRotation(InQuat);
}

void USceneComponent::SetLocalScale(const Vector3& InScale)
{
    if ((LocalTransform.Scale - InScale).LengthSquared() > TRANSFORM_EPSILON)
    {
        LocalTransform.Scale = InScale;
        OnLocalTransformChanged();
    }
}

void USceneComponent::AddLocalPosition(const Vector3& InDeltaPosition)
{
    if (InDeltaPosition.LengthSquared() > TRANSFORM_EPSILON)
    {
        Vector3 New = GetLocalPosition() + InDeltaPosition;
        SetLocalPosition(New);
    }
    
}

void USceneComponent::AddLocalRotation(const Quaternion& InDeltaRotation)
{
    // 변화량이 의미 있는지 확인
    float Dot = Quaternion::Dot(Quaternion::Identity, InDeltaRotation);
    float ChangeMagnitude = std::abs(1.0f - std::abs(Dot));

    if (ChangeMagnitude > TRANSFORM_EPSILON)
    {
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
}

void USceneComponent::AddLocalRotationEuler(const Vector3& InDeltaEuler)
{
    Quaternion Delta = Math::EulerToQuaternion(InDeltaEuler);
    AddLocalRotation(Delta);
}

// 월드 트랜스폼 설정자
void USceneComponent::SetWorldTransform(const FTransform& InWorldTransform)
{
    // 1. 나의 월드 변경
    WorldTransform = InWorldTransform;
    OnWorldTransformChanged();
}

void USceneComponent::SetWorldPosition(const Vector3& InWorldPosition)
{
 
    if ((WorldTransform.Position - InWorldPosition).LengthSquared() > TRANSFORM_EPSILON)
    {
        FTransform NewWorldTransform = WorldTransform;
        NewWorldTransform.Position = InWorldPosition;

        SetWorldTransform(NewWorldTransform);
    }
}

void USceneComponent::SetWorldRotation(const Quaternion& InWorldRotation)
{
     // 변화량이 의미 있는지 확인
    float Dot = Quaternion::Dot(Quaternion::Identity, InWorldRotation);
    float ChangeMagnitude = std::abs(1.0f - std::abs(Dot));

    if (ChangeMagnitude > TRANSFORM_EPSILON)
    {
        FTransform NewWorldTransform = WorldTransform;

         // 정규화 및 저장
        XMVECTOR RotQuat = XMLoadFloat4(&InWorldRotation);
        XMVECTOR ResultQuat = XMQuaternionNormalize(RotQuat);
        XMStoreFloat4(&NewWorldTransform.Rotation, ResultQuat);

        SetWorldTransform(NewWorldTransform);
    }
}

void USceneComponent::SetWorldRotationEuler(const Vector3& InWorldEuler)
{
    Quaternion InQuat = Math::EulerToQuaternion(InWorldEuler);
    SetWorldRotation(InQuat);
}

void USceneComponent::SetWorldScale(const Vector3& InWorldScale)
{
    if ((WorldTransform.Scale - InWorldScale).LengthSquared() > TRANSFORM_EPSILON)
    {
        FTransform NewWorldTransform = WorldTransform;
        NewWorldTransform.Scale = InWorldScale;
        SetWorldTransform(NewWorldTransform);
    }
}

void USceneComponent::AddWorldPosition(const Vector3& InDeltaPosition)
{
    Vector3  NewPosition = WorldTransform.Position + InDeltaPosition;
    SetWorldPosition(NewPosition);
}

void USceneComponent::AddWorldRotation(const Quaternion& InDeltaRotation)
{
    // 변화량이 의미 있는지 확인
    float Dot = Quaternion::Dot(Quaternion::Identity, InDeltaRotation);
    float ChangeMagnitude = std::abs(1.0f - std::abs(Dot));

    if (ChangeMagnitude > TRANSFORM_EPSILON)
    {
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
        auto SceneChild = std::dynamic_pointer_cast<USceneComponent>(Child);
        if (SceneChild)
        {
            auto NewChildWorldTransform = SceneChild->LocalToWorld(GetWorldTransform());
            SceneChild->SetWorldTransform(NewChildWorldTransform);
        }
    }
}

//void USceneComponent::MarkWorldTransformDirty()
//{
//    bWorldTransformDirty = true;
//}

const FTransform& USceneComponent::GetWorldTransform() const
{
    return WorldTransform;
}

const FTransform& USceneComponent::GetLocalTransform() const
{
    return LocalTransform;
}

void USceneComponent::LookAt(const Vector3& TargetWorldPosition)
{
   // 현재 월드 위치 가져오기
    Vector3 WorldPos = GetWorldPosition();

    // 타겟 방향 벡터 계산
    Vector3 Direction = TargetWorldPosition - WorldPos;

    // 방향 벡터가 너무 작으면 회전하지 않음
    if (Direction.LengthSquared() < TRANSFORM_EPSILON)
        return;

    // 방향을 정규화
    Direction.Normalize();

    // 기본적으로 로컬 전방 벡터는 (0,0,1)이고, 상향 벡터는 (0,1,0)
    Vector3 Forward = Vector3::Forward;
    Vector3 Up = Vector3::Up;

    // 방향에 맞는 회전 쿼터니언 생성
    Quaternion NewRotation = Quaternion::LookRotation(Direction, Up);

    // 월드 회전 설정
    SetWorldRotation(NewRotation);
}

void USceneComponent::RotateAroundAxis(const Vector3& Axis, float AngleDegrees)
{
   // 각도가 너무 작거나 축이 너무 작으면 회전하지 않음
    if (std::abs(AngleDegrees) < TRANSFORM_EPSILON || Axis.LengthSquared() < TRANSFORM_EPSILON)
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
        // 부모가 없는 경우, 월드
        UActorComponent::SetParent(nullptr);
        LocalTransform = FTransform(); //초기화(0)
        WorldTransform = CurrentWorldTransform;
    }
}