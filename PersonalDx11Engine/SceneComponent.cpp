#include "SceneComponent.h"

// 로컬 위치 설정
void USceneComponent::SetLocalPosition(const Vector3& InPosition)
{
    if ((LocalTransform.Position - InPosition).LengthSquared() > POSITION_THRESHOLD)
    {
        LocalTransform.Position = InPosition;
        MarkLocalTransformDirty();
    }
}

// 로컬 회전 설정
void USceneComponent::SetLocalRotation(const Quaternion& InRotation)
{
    // 회전 변화량 계산
    float Dot = Quaternion::Dot(LocalTransform.Rotation, InRotation);
    float ChangeMagnitude = std::abs(1.0f - std::abs(Dot));

    if (ChangeMagnitude > ROTATION_THRESHOLD)
    {
        float LengthSq = InRotation.LengthSquared();
        // 정규화 필요성 확인
        if (std::abs(LengthSq - 1.0f) > KINDA_SMALL)
        {
            LocalTransform.Rotation = InRotation.GetNormalized();
        }
        else
        {
            LocalTransform.Rotation = InRotation;
        }
        MarkLocalTransformDirty();
    }
}

// 로컬 오일러 회전
void USceneComponent::SetLocalRotationEuler(const Vector3& InEuler)
{
    const Quaternion Rot = Math::EulerToQuaternion(InEuler);
    SetLocalRotation(Rot);
}

// 로컬 스케일 설정
void USceneComponent::SetLocalScale(const Vector3& InScale)
{
    if ((LocalTransform.Scale - InScale).LengthSquared() > SCALE_THRESHOLD)
    {
        LocalTransform.Scale = InScale;
        MarkLocalTransformDirty();
    }
}

// 로컬 트랜스폼 한번에 설정
void USceneComponent::SetLocalTransform(const FTransform& InTransform)
{
    bool bChanged = false;

    // 위치 변경 확인
    if ((LocalTransform.Position - InTransform.Position).LengthSquared() > POSITION_THRESHOLD)
    {
        LocalTransform.Position = InTransform.Position;
        bChanged = true;
    }

    // 회전 변경 확인
    float Dot = Quaternion::Dot(LocalTransform.Rotation, InTransform.Rotation);
    float ChangeMagnitude = std::abs(1.0f - std::abs(Dot));

    if (ChangeMagnitude > ROTATION_THRESHOLD)
    {
        LocalTransform.Rotation = InTransform.Rotation;
        bChanged = true;
    }

    // 스케일 변경 확인
    if ((LocalTransform.Scale - InTransform.Scale).LengthSquared() > SCALE_THRESHOLD)
    {
        LocalTransform.Scale = InTransform.Scale;
        bChanged = true;
    }

    // 변경사항이 있을 경우만 갱신
    if (bChanged)
    {
        MarkLocalTransformDirty();
    }
}

// 로컬 위치 반환
const Vector3& USceneComponent::GetLocalPosition() const
{
    return LocalTransform.Position;
}

// 로컬 회전 반환
const Quaternion& USceneComponent::GetLocalRotation() const
{
    return LocalTransform.Rotation;
}

// 로컬 스케일 반환
const Vector3& USceneComponent::GetLocalScale() const
{
    return LocalTransform.Scale;
}

// 로컬 위치에 추가
void USceneComponent::AddLocalPosition(const Vector3& InDeltaPosition)
{
    if (InDeltaPosition.LengthSquared() > POSITION_THRESHOLD)
    {
        LocalTransform.Position += InDeltaPosition;
        MarkLocalTransformDirty();
    }
}

// 로컬 회전에 추가 (오일러 각도)
void USceneComponent::AddLocalRotationEuler(const Vector3& InDeltaEuler)
{
    Quaternion DeltaRotation = Math::EulerToQuaternion(InDeltaEuler);
    AddLocalRotation(DeltaRotation);
}

// 로컬 회전에 추가 (쿼터니언)
void USceneComponent::AddLocalRotation(const Quaternion& InDeltaRotation)
{
    // 변화량이 의미 있는지 확인
    float Dot = Quaternion::Dot(Quaternion::Identity, InDeltaRotation);
    float ChangeMagnitude = std::abs(1.0f - std::abs(Dot));

    if (ChangeMagnitude > ROTATION_THRESHOLD)
    {
        // 현재 회전에 새 회전을 합성
        XMVECTOR CurrentV = XMLoadFloat4(&LocalTransform.Rotation);
        XMVECTOR DeltaV = XMLoadFloat4(&InDeltaRotation);

        // 쿼터니언 곱셈으로 회전 결합
        XMVECTOR ResultV = XMQuaternionMultiply(CurrentV, DeltaV);
        ResultV = XMQuaternionNormalize(ResultV);

        Quaternion Result;
        XMStoreFloat4(&Result, ResultV);

        LocalTransform.Rotation = Result;
        MarkLocalTransformDirty();
    }
}

// 주어진 축을 중심으로 회전
void USceneComponent::RotateLocalAroundAxis(const Vector3& InAxis, float AngleDegrees)
{
    if (AngleDegrees <= KINDA_SMALL || InAxis.LengthSquared() < KINDA_SMALL)
        return;

    // 회전축 정규화
    Vector3 NormalizedAxis = InAxis.GetNormalized();

    // 회전 쿼터니언 생성
    float AngleRadians = Math::DegreeToRad(AngleDegrees);

    XMVECTOR Axis = XMLoadFloat3(&NormalizedAxis);
    XMVECTOR DeltaRotation = XMQuaternionRotationAxis(Axis, AngleRadians);

    // 현재 회전에 적용
    XMVECTOR CurrentQuat = XMLoadFloat4(&LocalTransform.Rotation);
    XMVECTOR FinalQuat = XMQuaternionMultiply(DeltaRotation, CurrentQuat);

    // 결과 정규화 및 저장
    FinalQuat = XMQuaternionNormalize(FinalQuat);

    Quaternion Result;
    XMStoreFloat4(&Result, FinalQuat);

    SetLocalRotation(Result);
}

// 월드 트랜스폼 계산 및 반환
const FTransform& USceneComponent::GetWorldTransform() const
{
    UpdateWorldTransformIfNeeded();
    return CachedWorldTransform;
}

// 월드 위치 반환
Vector3 USceneComponent::GetWorldPosition() const
{
    UpdateWorldTransformIfNeeded();
    return CachedWorldTransform.Position;
}

// 월드 회전 반환
Quaternion USceneComponent::GetWorldRotation() const
{
    UpdateWorldTransformIfNeeded();
    return CachedWorldTransform.Rotation;
}

// 월드 스케일 반환 
Vector3 USceneComponent::GetWorldScale() const
{
    UpdateWorldTransformIfNeeded();
    return CachedWorldTransform.Scale;
}

// 특정 월드 위치를 바라보게 설정
void USceneComponent::LookAt(const Vector3& WorldTargetPos)
{
    Vector3 WorldPos = GetWorldPosition();
    Vector3 Direction = WorldTargetPos - WorldPos;

    if (Direction.LengthSquared() < KINDA_SMALL)
        return;

    Direction.Normalize();

    // 기본 업 벡터
    Vector3 Up = Vector3::Up;

    // World 기준 Forward는 Z축 (언리얼과 같은 좌표계 기준)
    Vector3 Forward = Vector3::Forward;

    // 현재 월드 회전 구하기
    Quaternion WorldRot = GetWorldRotation();

    // 목표 방향으로의 최소 회전 계산
    Quaternion LookRotation = Vector4::LookRotation(Direction, Up);

    // World 회전을 Local 회전으로 변환
    // 부모의 역회전 필요
    auto Parent = GetSceneParent();
    if (Parent)
    {
        Quaternion ParentWorldRot = Parent->GetWorldRotation();

        // 부모의 역회전 (inverse)을 구해 적용
        XMVECTOR ParentWorldRotV = XMLoadFloat4(&ParentWorldRot);
        XMVECTOR ParentWorldRotInvV = XMQuaternionInverse(ParentWorldRotV);

        XMVECTOR LookRotationV = XMLoadFloat4(&LookRotation);
        XMVECTOR LocalRotV = XMQuaternionMultiply(ParentWorldRotInvV, LookRotationV);

        Quaternion LocalRot;
        XMStoreFloat4(&LocalRot, LocalRotV);

        SetLocalRotation(LocalRot);
    }
    else
    {
        // 부모가 없으면 월드 회전이 로컬 회전
        SetLocalRotation(LookRotation);
    }
}

// 월드 위치 직접 설정 (내부적으로 로컬로 변환)
void USceneComponent::SetWorldPosition(const Vector3& WorldPosition)
{
    auto Parent = GetSceneParent();
    if (Parent)
    {
        // 부모 기준으로 로컬 위치 계산
        FTransform ParentWorld = Parent->GetWorldTransform();

        // 부모의 역변환 계산하여 적용
        XMVECTOR WorldPosV = XMLoadFloat3(&WorldPosition);
        XMVECTOR ParentPosV = XMLoadFloat3(&ParentWorld.Position);
        XMVECTOR ParentRotV = XMLoadFloat4(&ParentWorld.Rotation);
        XMVECTOR ParentScaleV = XMLoadFloat3(&ParentWorld.Scale);

        // 부모 위치로부터의 차이
        XMVECTOR RelativePosV = XMVectorSubtract(WorldPosV, ParentPosV);

        // 부모 회전의 역회전
        XMVECTOR ParentRotInvV = XMQuaternionInverse(ParentRotV);

        // 역회전 적용
        XMVECTOR LocalPosRotatedV = XMVector3Rotate(RelativePosV, ParentRotInvV);

        // 스케일 역적용 (나누기)
        XMVECTOR InvScaleV = XMVectorReciprocal(ParentScaleV);
        XMVECTOR LocalPosV = XMVectorMultiply(LocalPosRotatedV, InvScaleV);

        Vector3 LocalPos;
        XMStoreFloat3(&LocalPos, LocalPosV);

        SetLocalPosition(LocalPos);
    }
    else
    {
        // 부모가 없으면 월드 위치가 로컬 위치
        SetLocalPosition(WorldPosition);
    }
}

// 로컬 트랜스폼 상태 갱신을 필요로 표시
void USceneComponent::MarkLocalTransformDirty()
{
    LocalTransformVersion++;
    WorldTransformVersion = 0; // 월드 트랜스폼 캐시 무효화

    // 트랜스폼 변경 알림
    OnTransformChangedDelegate.Broadcast(LocalTransform);

    // 자식 컴포넌트들에게 부모 변경 알림
    NotifyChildrenOfTransformChange();
}

// 월드 트랜스폼 상태 갱신이 필요한지 확인하고 필요시 업데이트
void USceneComponent::UpdateWorldTransformIfNeeded() const
{
    // 업데이트가 필요한지 확인
    bool bNeedsUpdate = IsWorldTransformDirty();
    auto Parent = GetSceneParent();
    if (Parent)
    {
        uint32_t CurrentParentVersion = Parent->GetWorldTransformVersion();
        bNeedsUpdate = bNeedsUpdate || (ParentWorldTransformVersion != CurrentParentVersion);
    }

    if (bNeedsUpdate)
    {
        CalculateWorldTransform();
    }
}

// 월드 트랜스폼 실제 계산
void USceneComponent::CalculateWorldTransform() const
{
    auto Parent = GetSceneParent();

    if (Parent)
    {
        // 부모의 월드 트랜스폼 가져오기
        const FTransform& ParentWorld = Parent->GetWorldTransform();

        // 상속된 트랜스폼 계산

        // 1. 스케일: 단순 곱셈
		CachedWorldTransform.Scale ={ ParentWorld.Scale.x * LocalTransform.Scale.x,
			ParentWorld.Scale.y * LocalTransform.Scale.y,
			ParentWorld.Scale.z * LocalTransform.Scale.z };

        // 2. 회전: 쿼터니언 곱셈
        XMVECTOR ParentRotV = XMLoadFloat4(&ParentWorld.Rotation);
        XMVECTOR LocalRotV = XMLoadFloat4(&LocalTransform.Rotation);
        XMVECTOR WorldRotV = XMQuaternionMultiply(ParentRotV, LocalRotV);
        XMStoreFloat4(&CachedWorldTransform.Rotation, WorldRotV);

        // 3. 위치: 부모 위치 + (부모 회전 * 부모 스케일 * 로컬 위치)
        XMVECTOR LocalPosV = XMLoadFloat3(&LocalTransform.Position);
        XMVECTOR ParentScaleV = XMLoadFloat3(&ParentWorld.Scale);

        // 스케일 적용
        XMVECTOR ScaledLocalPosV = XMVectorMultiply(LocalPosV, ParentScaleV);

        // 회전 적용
        XMVECTOR RotatedScaledLocalPosV = XMVector3Rotate(ScaledLocalPosV, ParentRotV);

        // 부모 위치에 더하기
        XMVECTOR ParentPosV = XMLoadFloat3(&ParentWorld.Position);
        XMVECTOR WorldPosV = XMVectorAdd(ParentPosV, RotatedScaledLocalPosV);

        XMStoreFloat3(&CachedWorldTransform.Position, WorldPosV);

        // 부모 버전 저장
        ParentWorldTransformVersion = Parent->GetWorldTransformVersion();
    }
    else
    {
        // 부모가 없으면 로컬 트랜스폼이 월드 트랜스폼
        CachedWorldTransform = LocalTransform;
    }

    // 월드 트랜스폼 버전 업데이트
    WorldTransformVersion = LocalTransformVersion;
}

// 자식 컴포넌트들에게 트랜스폼 변경 알림
void USceneComponent::NotifyChildrenOfTransformChange()
{
    auto SceneChildren = FindChildrenByType<USceneComponent>();
    for (const auto& SceneChild : SceneChildren)
    {
        if (SceneChild.lock())
        {
            SceneChild.lock()->OnParentTransformChanged();
        }
    }
}

// 부모 트랜스폼 변경 처리
void USceneComponent::OnParentTransformChanged()
{
    // 월드 트랜스폼 캐시 무효화
    WorldTransformVersion = 0;

    // 변경 사항을 자식들에게 전파
    NotifyChildrenOfTransformChange();
}
