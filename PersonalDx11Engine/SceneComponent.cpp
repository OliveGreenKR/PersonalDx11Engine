#include "SceneComponent.h"

// 로컬 트랜스폼 설정자
void USceneComponent::SetLocalTransform(const FTransform& InTransform)
{
    // 현재 트랜스폼과 동일한지 검사하여 불필요한 업데이트 방지
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
        MarkLocalTransformDirty();
    }
}

void USceneComponent::SetLocalPosition(const Vector3& InPosition)
{
    if ((LocalTransform.Position - InPosition).LengthSquared() > TRANSFORM_EPSILON)
    {
        LocalTransform.Position = InPosition;
        MarkLocalTransformDirty();
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
        MarkLocalTransformDirty();
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
        MarkLocalTransformDirty();
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
        MarkLocalTransformDirty();
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
    bWorldTransformDirty = false;

    // 2. 나의 로컬 변경
    UpdateLocalTransform();

    // 3. 자식의 월드 변경
    PropagateWorldTransformToChildren();

    // 이벤트 발생
    OnWorldTransformChangedDelegate.Broadcast(WorldTransform);
}

void USceneComponent::SetWorldPosition(const Vector3& InWorldPosition)
{
    // 현재 월드 트랜스폼이 최신 상태가 아니면 업데이트
    if (bWorldTransformDirty)
    {
        UpdateWorldTransform();
    }

    if ((WorldTransform.Position - InWorldPosition).LengthSquared() > TRANSFORM_EPSILON)
    {
        FTransform NewWorldTransform = WorldTransform;
        NewWorldTransform.Position = InWorldPosition;

        SetWorldTransform(NewWorldTransform);
    }
}

void USceneComponent::SetWorldRotation(const Quaternion& InWorldRotation)
{
    // 현재 월드 트랜스폼이 최신 상태가 아니면 업데이트
    if (bWorldTransformDirty)
    {
        UpdateWorldTransform();
    }

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
    // 현재 월드 트랜스폼이 최신 상태가 아니면 업데이트
    if (bWorldTransformDirty)
    {
        UpdateWorldTransform();
    }
    if ((WorldTransform.Scale - InWorldScale).LengthSquared() > TRANSFORM_EPSILON)
    {
        FTransform NewWorldTransform = WorldTransform;
        NewWorldTransform.Scale = InWorldScale;
        SetWorldTransform(NewWorldTransform);
    }
}

void USceneComponent::AddWorldPosition(const Vector3& InDeltaPosition)
{
    if (bWorldTransformDirty)
    {
        UpdateWorldTransform();
    }
    Vector3  NewPosition = WorldTransform.Position + InDeltaPosition;
    SetWorldPosition(NewPosition);
}

void USceneComponent::AddWorldRotation(const Quaternion& InDeltaRotation)
{
    if (bWorldTransformDirty)
    {
        UpdateWorldTransform();
    }

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

// 트랜스폼 업데이트 함수
void USceneComponent::UpdateWorldTransform() const
{
    if (!bWorldTransformDirty)
        return;

    auto Parent = GetSceneParent();
    if (Parent)
    {
        // 부모의 월드 트랜스폼 가져오기
        const FTransform& ParentWorldTransform = Parent->GetWorldTransform();

        // 로컬에서 월드로 변환
        WorldTransform = LocalToWorld(ParentWorldTransform);
    }
    else
    {
        // 부모가 없는 경우 로컬 트랜스폼이 월드 트랜스폼
        WorldTransform = LocalTransform;
    }

    bWorldTransformDirty = false;
}

void USceneComponent::UpdateLocalTransform()
{
    auto Parent = GetSceneParent();
    if (Parent)
    {
        // 부모의 월드 트랜스폼 가져오기
        const FTransform& ParentWorldTransform = Parent->GetWorldTransform();

        // 월드에서 로컬로 변환
        LocalTransform = WorldToLocal(ParentWorldTransform);
    }
    else
    {
        // 부모가 없는 경우 월드 트랜스폼이 로컬 트랜스폼
        LocalTransform = WorldTransform;
    }

    bLocalTransformDirty = false;

    // 이벤트 발생
    OnLocalTransformChangedDelegate.Broadcast(LocalTransform);
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
            SceneChild->MarkWorldTransformDirty();
            SceneChild->PropagateWorldTransformToChildren();
        }
    }
}

// 플래그 설정 함수
void USceneComponent::MarkLocalTransformDirty()
{
    bLocalTransformDirty = true;
    bWorldTransformDirty = true;

    // 이벤트 발생
    OnLocalTransformChangedDelegate.Broadcast(LocalTransform);

    // 자식들의 월드 트랜스폼도 더티로 표시
    PropagateWorldTransformToChildren();
}

void USceneComponent::MarkWorldTransformDirty()
{
    bWorldTransformDirty = true;
}

// 트랜스폼 접근자
const FTransform& USceneComponent::GetWorldTransform() const
{
    if (bWorldTransformDirty)
    {
        UpdateWorldTransform();
    }
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

    // 기존 부모에서 분리
    auto OldParent = GetParent();
    if (OldParent)
    {
        UActorComponent::SetParent(nullptr);
    }

    // 새 부모가 있으면 연결
    if (InParent)
    {
        // 부모-자식 관계 설정
        UActorComponent::SetParent(InParent);

        // 월드 트랜스폼 유지를 위한 로컬 트랜스폼 계산
        WorldTransform = CurrentWorldTransform;
        bWorldTransformDirty = false;
        UpdateLocalTransform();
    }
    else
    {
        // 부모가 없는 경우, 월드 = 로컬
        UActorComponent::SetParent(nullptr);
        LocalTransform = CurrentWorldTransform;
        WorldTransform = CurrentWorldTransform;
        bLocalTransformDirty = false;
        bWorldTransformDirty = false;
    }
}

static FTransform LocalToWorld(const FTransform& ChildLocal, const FTransform& ParentWorld)
{
    FTransform Result;

    // 스케일: 부모 스케일 * 자식 로컬 스케일
    Result.Scale = Vector3(
        ParentWorld.Scale.x * ChildLocal.Scale.x,
        ParentWorld.Scale.y * ChildLocal.Scale.y,
        ParentWorld.Scale.z * ChildLocal.Scale.z
    );

    // 회전: 부모 회전 * 자식 로컬 회전
    XMVECTOR ParentRot = XMLoadFloat4(&ParentWorld.Rotation);
    XMVECTOR LocalRot = XMLoadFloat4(&ChildLocal.Rotation);
    XMVECTOR WorldRot = XMQuaternionMultiply(ParentRot, LocalRot);
    XMStoreFloat4(&Result.Rotation, WorldRot);

    // 위치: 부모 위치 + (부모 회전 * 부모 스케일 * 자식 로컬 위치)
    XMVECTOR LocalPos = XMLoadFloat3(&ChildLocal.Position);
    XMVECTOR ParentScale = XMLoadFloat3(&ParentWorld.Scale);

    // 스케일 적용
    XMVECTOR ScaledPos = XMVectorMultiply(LocalPos, ParentScale);

    // 회전 적용
    XMVECTOR RotatedPos = XMVector3Rotate(ScaledPos, ParentRot);

    // 부모 위치에 더하기
    XMVECTOR ParentPos = XMLoadFloat3(&ParentWorld.Position);
    XMVECTOR WorldPos = XMVectorAdd(ParentPos, RotatedPos);

    XMStoreFloat3(&Result.Position, WorldPos);

    return Result;
}

static FTransform WorldToLocal(const FTransform& ChildWorld, const FTransform& ParentWorld)
{
    FTransform Result;

    // 스케일: 자식 월드 스케일 / 부모 스케일
    Result.Scale = Vector3(
        ChildWorld.Scale.x / ParentWorld.Scale.x,
        ChildWorld.Scale.y / ParentWorld.Scale.y,
        ChildWorld.Scale.z / ParentWorld.Scale.z
    );

    // 회전: 부모 회전의 역 * 자식 월드 회전
    XMVECTOR ParentRot = XMLoadFloat4(&ParentWorld.Rotation);
    XMVECTOR WorldRot = XMLoadFloat4(&ChildWorld.Rotation);
    XMVECTOR InvParentRot = XMQuaternionInverse(ParentRot);
    XMVECTOR LocalRot = XMQuaternionMultiply(InvParentRot, WorldRot);
    XMStoreFloat4(&Result.Rotation, LocalRot);

    // 위치: (부모 회전의 역 * (자식 월드 위치 - 부모 위치)) / 부모 스케일
    XMVECTOR WorldPos = XMLoadFloat3(&ChildWorld.Position);
    XMVECTOR ParentPos = XMLoadFloat3(&ParentWorld.Position);
    XMVECTOR RelativePos = XMVectorSubtract(WorldPos, ParentPos);

    // 부모 회전의 역 적용
    XMVECTOR UnrotatedPos = XMVector3Rotate(RelativePos, InvParentRot);

    // 부모 스케일의 역 적용
    XMVECTOR InvParentScale = XMVectorReciprocal(XMLoadFloat3(&ParentWorld.Scale));
    XMVECTOR LocalPos = XMVectorMultiply(UnrotatedPos, InvParentScale);

    XMStoreFloat3(&Result.Position, LocalPos);

    return Result;
}