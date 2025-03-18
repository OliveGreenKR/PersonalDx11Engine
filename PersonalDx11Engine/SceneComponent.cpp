#include "SceneComponent.h"

// ���� ��ġ ����
void USceneComponent::SetLocalPosition(const Vector3& InPosition)
{
    if ((LocalTransform.Position - InPosition).LengthSquared() > POSITION_THRESHOLD)
    {
        LocalTransform.Position = InPosition;
        MarkLocalTransformDirty();
    }
}

// ���� ȸ�� ����
void USceneComponent::SetLocalRotation(const Quaternion& InRotation)
{
    // ȸ�� ��ȭ�� ���
    float Dot = Quaternion::Dot(LocalTransform.Rotation, InRotation);
    float ChangeMagnitude = std::abs(1.0f - std::abs(Dot));

    if (ChangeMagnitude > ROTATION_THRESHOLD)
    {
        float LengthSq = InRotation.LengthSquared();
        // ����ȭ �ʿ伺 Ȯ��
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

// ���� ���Ϸ� ȸ��
void USceneComponent::SetLocalRotationEuler(const Vector3& InEuler)
{
    const Quaternion Rot = Math::EulerToQuaternion(InEuler);
    SetLocalRotation(Rot);
}

// ���� ������ ����
void USceneComponent::SetLocalScale(const Vector3& InScale)
{
    if ((LocalTransform.Scale - InScale).LengthSquared() > SCALE_THRESHOLD)
    {
        LocalTransform.Scale = InScale;
        MarkLocalTransformDirty();
    }
}

// ���� Ʈ������ �ѹ��� ����
void USceneComponent::SetLocalTransform(const FTransform& InTransform)
{
    bool bChanged = false;

    // ��ġ ���� Ȯ��
    if ((LocalTransform.Position - InTransform.Position).LengthSquared() > POSITION_THRESHOLD)
    {
        LocalTransform.Position = InTransform.Position;
        bChanged = true;
    }

    // ȸ�� ���� Ȯ��
    float Dot = Quaternion::Dot(LocalTransform.Rotation, InTransform.Rotation);
    float ChangeMagnitude = std::abs(1.0f - std::abs(Dot));

    if (ChangeMagnitude > ROTATION_THRESHOLD)
    {
        LocalTransform.Rotation = InTransform.Rotation;
        bChanged = true;
    }

    // ������ ���� Ȯ��
    if ((LocalTransform.Scale - InTransform.Scale).LengthSquared() > SCALE_THRESHOLD)
    {
        LocalTransform.Scale = InTransform.Scale;
        bChanged = true;
    }

    // ��������� ���� ��츸 ����
    if (bChanged)
    {
        MarkLocalTransformDirty();
    }
}

// ���� ��ġ ��ȯ
const Vector3& USceneComponent::GetLocalPosition() const
{
    return LocalTransform.Position;
}

// ���� ȸ�� ��ȯ
const Quaternion& USceneComponent::GetLocalRotation() const
{
    return LocalTransform.Rotation;
}

// ���� ������ ��ȯ
const Vector3& USceneComponent::GetLocalScale() const
{
    return LocalTransform.Scale;
}

// ���� ��ġ�� �߰�
void USceneComponent::AddLocalPosition(const Vector3& InDeltaPosition)
{
    if (InDeltaPosition.LengthSquared() > POSITION_THRESHOLD)
    {
        LocalTransform.Position += InDeltaPosition;
        MarkLocalTransformDirty();
    }
}

// ���� ȸ���� �߰� (���Ϸ� ����)
void USceneComponent::AddLocalRotationEuler(const Vector3& InDeltaEuler)
{
    Quaternion DeltaRotation = Math::EulerToQuaternion(InDeltaEuler);
    AddLocalRotation(DeltaRotation);
}

// ���� ȸ���� �߰� (���ʹϾ�)
void USceneComponent::AddLocalRotation(const Quaternion& InDeltaRotation)
{
    // ��ȭ���� �ǹ� �ִ��� Ȯ��
    float Dot = Quaternion::Dot(Quaternion::Identity, InDeltaRotation);
    float ChangeMagnitude = std::abs(1.0f - std::abs(Dot));

    if (ChangeMagnitude > ROTATION_THRESHOLD)
    {
        // ���� ȸ���� �� ȸ���� �ռ�
        XMVECTOR CurrentV = XMLoadFloat4(&LocalTransform.Rotation);
        XMVECTOR DeltaV = XMLoadFloat4(&InDeltaRotation);

        // ���ʹϾ� �������� ȸ�� ����
        XMVECTOR ResultV = XMQuaternionMultiply(CurrentV, DeltaV);
        ResultV = XMQuaternionNormalize(ResultV);

        Quaternion Result;
        XMStoreFloat4(&Result, ResultV);

        LocalTransform.Rotation = Result;
        MarkLocalTransformDirty();
    }
}

// �־��� ���� �߽����� ȸ��
void USceneComponent::RotateLocalAroundAxis(const Vector3& InAxis, float AngleDegrees)
{
    if (AngleDegrees <= KINDA_SMALL || InAxis.LengthSquared() < KINDA_SMALL)
        return;

    // ȸ���� ����ȭ
    Vector3 NormalizedAxis = InAxis.GetNormalized();

    // ȸ�� ���ʹϾ� ����
    float AngleRadians = Math::DegreeToRad(AngleDegrees);

    XMVECTOR Axis = XMLoadFloat3(&NormalizedAxis);
    XMVECTOR DeltaRotation = XMQuaternionRotationAxis(Axis, AngleRadians);

    // ���� ȸ���� ����
    XMVECTOR CurrentQuat = XMLoadFloat4(&LocalTransform.Rotation);
    XMVECTOR FinalQuat = XMQuaternionMultiply(DeltaRotation, CurrentQuat);

    // ��� ����ȭ �� ����
    FinalQuat = XMQuaternionNormalize(FinalQuat);

    Quaternion Result;
    XMStoreFloat4(&Result, FinalQuat);

    SetLocalRotation(Result);
}

// ���� Ʈ������ ��� �� ��ȯ
const FTransform& USceneComponent::GetWorldTransform() const
{
    UpdateWorldTransformIfNeeded();
    return CachedWorldTransform;
}

// ���� ��ġ ��ȯ
Vector3 USceneComponent::GetWorldPosition() const
{
    UpdateWorldTransformIfNeeded();
    return CachedWorldTransform.Position;
}

// ���� ȸ�� ��ȯ
Quaternion USceneComponent::GetWorldRotation() const
{
    UpdateWorldTransformIfNeeded();
    return CachedWorldTransform.Rotation;
}

// ���� ������ ��ȯ 
Vector3 USceneComponent::GetWorldScale() const
{
    UpdateWorldTransformIfNeeded();
    return CachedWorldTransform.Scale;
}

// Ư�� ���� ��ġ�� �ٶ󺸰� ����
void USceneComponent::LookAt(const Vector3& WorldTargetPos)
{
    Vector3 WorldPos = GetWorldPosition();
    Vector3 Direction = WorldTargetPos - WorldPos;

    if (Direction.LengthSquared() < KINDA_SMALL)
        return;

    Direction.Normalize();

    // �⺻ �� ����
    Vector3 Up = Vector3::Up;

    // World ���� Forward�� Z�� (�𸮾�� ���� ��ǥ�� ����)
    Vector3 Forward = Vector3::Forward;

    // ���� ���� ȸ�� ���ϱ�
    Quaternion WorldRot = GetWorldRotation();

    // ��ǥ ���������� �ּ� ȸ�� ���
    Quaternion LookRotation = Vector4::LookRotation(Direction, Up);

    // World ȸ���� Local ȸ������ ��ȯ
    // �θ��� ��ȸ�� �ʿ�
    auto Parent = GetSceneParent();
    if (Parent)
    {
        Quaternion ParentWorldRot = Parent->GetWorldRotation();

        // �θ��� ��ȸ�� (inverse)�� ���� ����
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
        // �θ� ������ ���� ȸ���� ���� ȸ��
        SetLocalRotation(LookRotation);
    }
}

// ���� ��ġ ���� ���� (���������� ���÷� ��ȯ)
void USceneComponent::SetWorldPosition(const Vector3& WorldPosition)
{
    auto Parent = GetSceneParent();
    if (Parent)
    {
        // �θ� �������� ���� ��ġ ���
        FTransform ParentWorld = Parent->GetWorldTransform();

        // �θ��� ����ȯ ����Ͽ� ����
        XMVECTOR WorldPosV = XMLoadFloat3(&WorldPosition);
        XMVECTOR ParentPosV = XMLoadFloat3(&ParentWorld.Position);
        XMVECTOR ParentRotV = XMLoadFloat4(&ParentWorld.Rotation);
        XMVECTOR ParentScaleV = XMLoadFloat3(&ParentWorld.Scale);

        // �θ� ��ġ�κ����� ����
        XMVECTOR RelativePosV = XMVectorSubtract(WorldPosV, ParentPosV);

        // �θ� ȸ���� ��ȸ��
        XMVECTOR ParentRotInvV = XMQuaternionInverse(ParentRotV);

        // ��ȸ�� ����
        XMVECTOR LocalPosRotatedV = XMVector3Rotate(RelativePosV, ParentRotInvV);

        // ������ ������ (������)
        XMVECTOR InvScaleV = XMVectorReciprocal(ParentScaleV);
        XMVECTOR LocalPosV = XMVectorMultiply(LocalPosRotatedV, InvScaleV);

        Vector3 LocalPos;
        XMStoreFloat3(&LocalPos, LocalPosV);

        SetLocalPosition(LocalPos);
    }
    else
    {
        // �θ� ������ ���� ��ġ�� ���� ��ġ
        SetLocalPosition(WorldPosition);
    }
}

// ���� Ʈ������ ���� ������ �ʿ�� ǥ��
void USceneComponent::MarkLocalTransformDirty()
{
    //LocalTransformVersion++;
    //WorldTransformVersion = 0; // ���� Ʈ������ ĳ�� ��ȿȭ

    // ������ Root�� �ϴ� ��� ����Ʈ���� ������Ʈ �ʿ伺 ����
    PropagateUpdateFlagToSubtree();

    // Ʈ������ ���� �̺�Ʈ �߻�
    OnTransformChangedDelegate.Broadcast(LocalTransform);
}

// ���� Ʈ������ ���� ���
void USceneComponent::CalculateWorldTransform() const
{
    auto Parent = GetSceneParent();

    if (Parent)
    {
        // �θ��� ���� Ʈ������ ��������
        const FTransform& ParentWorld = Parent->GetWorldTransform();

        // ��ӵ� Ʈ������ ���
        // 1. ������: XMVector Ȱ��
        XMVECTOR ParentScaleV = XMLoadFloat3(&ParentWorld.Scale);
        XMVECTOR LocalScaleV = XMLoadFloat3(&LocalTransform.Scale);
        XMVECTOR WorldScaleV = XMVectorMultiply(ParentScaleV, LocalScaleV);
        XMStoreFloat3(&CachedWorldTransform.Scale, WorldScaleV);

        // 2. ȸ��: ���� �ڵ� ���� (����)
        XMVECTOR ParentRotV = XMLoadFloat4(&ParentWorld.Rotation);
        XMVECTOR LocalRotV = XMLoadFloat4(&LocalTransform.Rotation);
        XMVECTOR WorldRotV = XMQuaternionMultiply(ParentRotV, LocalRotV);
        XMStoreFloat4(&CachedWorldTransform.Rotation, WorldRotV);

        // 3. ��ġ ��� ����ȭ
        XMVECTOR LocalPosV = XMLoadFloat3(&LocalTransform.Position);
        XMVECTOR ScaledLocalPosV = XMVectorMultiply(LocalPosV, WorldScaleV);
        XMVECTOR RotatedScaledLocalPosV = XMVector3Rotate(ScaledLocalPosV, ParentRotV);
        XMVECTOR ParentPosV = XMLoadFloat3(&ParentWorld.Position);
        XMVECTOR WorldPosV = XMVectorAdd(ParentPosV, RotatedScaledLocalPosV);
        XMStoreFloat3(&CachedWorldTransform.Position, WorldPosV);

        //// �θ� ���� ����
        //ParentWorldTransformVersion = Parent->GetWorldTransformVersion();
    }
    else
    {
        // �θ� ������ ���� Ʈ�������� ���� Ʈ������
        CachedWorldTransform = LocalTransform;
    }

    // ���� Ʈ������ ���� ������Ʈ
    //WorldTransformVersion = LocalTransformVersion;
}

// Root ������Ʈ ã��
USceneComponent* USceneComponent::FindRootSceneComponent() const
{
    const USceneComponent* Current = this;
    while (auto Parent = Current->GetSceneParent())
    {
        Current = Parent.get();
    }
    return const_cast<USceneComponent*>(Current);
}

// ����Ʈ���� ������Ʈ �ʿ伺 ����
void USceneComponent::PropagateUpdateFlagToSubtree() const
{
    // �̹� ������Ʈ �ʿ�� ǥ�õǾ� �ִٸ� ���� ���� (�̹� ���ĵ�)
    if (bNeedsWorldTransformUpdate)
        return;

    // ������Ʈ �ʿ� ǥ��
    bNeedsWorldTransformUpdate = true;

    // ��� �ڽ� ������Ʈ�� ����
    auto SceneChildren = FindChildrenByType<USceneComponent>();
    for (const auto& WeakChild : SceneChildren)
    {
        if (auto Child = WeakChild.lock())
        {
            Child->PropagateUpdateFlagToSubtree();
        }
    }
}

// UpdateWorldTransformIfNeeded �޼��� ����
void USceneComponent::UpdateWorldTransformIfNeeded() const
{
    // ������Ʈ�� �ʿ����� ������ �ٷ� ����
    if (!IsWorldTransfromNeedUpdate())
        return;

    // �θ� �ִٸ� �θ���� ������Ʈ
    auto Parent = GetSceneParent();
    if (Parent)
    {
        Parent->UpdateWorldTransformIfNeeded();

        // ���� Ʈ������ ���
        CalculateWorldTransform();
    }
    else
    {
        // Root ������Ʈ�� ���� ���
        CalculateWorldTransform();
    }

    // ������Ʈ �÷��� �ʱ�ȭ
    bNeedsWorldTransformUpdate = false;
}

// OnParentTransformChanged �޼��� ����
void USceneComponent::OnParentTransformChanged()
{
    //// ���� Ʈ������ ĳ�� ��ȿȭ
    //WorldTransformVersion = 0;

    // ������ Root�� �ϴ� ��� ����Ʈ���� ������Ʈ �ʿ伺 ����
    PropagateUpdateFlagToSubtree();
}