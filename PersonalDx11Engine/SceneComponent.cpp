#include "SceneComponent.h"

// ���� Ʈ������ ������
void USceneComponent::SetLocalTransform(const FTransform& InTransform)
{
    // ���� Ʈ�������� �������� �˻��Ͽ� ���ʿ��� ������Ʈ ����
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
     // ��ȭ���� �ǹ� �ִ��� Ȯ��
    float Dot = Quaternion::Dot(Quaternion::Identity, InRotation);
    float ChangeMagnitude = std::abs(1.0f - std::abs(Dot));

    if (ChangeMagnitude > TRANSFORM_EPSILON)
    {
         // ����ȭ �� ����
        XMVECTOR RotQuat = XMLoadFloat4(&InRotation);
        XMVECTOR ResultQuat = XMQuaternionNormalize(RotQuat);
        XMStoreFloat4(&LocalTransform.Rotation, ResultQuat);

        // ���� �÷��� ���� �� ����
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
    // ��ȭ���� �ǹ� �ִ��� Ȯ��
    float Dot = Quaternion::Dot(Quaternion::Identity, InDeltaRotation);
    float ChangeMagnitude = std::abs(1.0f - std::abs(Dot));

    if (ChangeMagnitude > TRANSFORM_EPSILON)
    {
        // ���� ȸ���� �� ȸ���� �ռ�
        XMVECTOR CurrentQuat = XMLoadFloat4(&LocalTransform.Rotation);
        XMVECTOR DeltaQuat = XMLoadFloat4(&InDeltaRotation);

        // ���ʹϾ� ������ ȸ�� ���� (���� �߿�: ��Ÿ * ����)
        XMVECTOR ResultQuat = XMQuaternionMultiply(DeltaQuat, CurrentQuat);

        // ����ȭ �� ����
        ResultQuat = XMQuaternionNormalize(ResultQuat);
        XMStoreFloat4(&LocalTransform.Rotation, ResultQuat);

        // ���� �÷��� ���� �� ����
        MarkLocalTransformDirty();
    }
}

void USceneComponent::AddLocalRotationEuler(const Vector3& InDeltaEuler)
{
    Quaternion Delta = Math::EulerToQuaternion(InDeltaEuler);
    AddLocalRotation(Delta);
}

// ���� Ʈ������ ������
void USceneComponent::SetWorldTransform(const FTransform& InWorldTransform)
{
    // 1. ���� ���� ����
    WorldTransform = InWorldTransform;
    bWorldTransformDirty = false;

    // 2. ���� ���� ����
    UpdateLocalTransform();

    // 3. �ڽ��� ���� ����
    PropagateWorldTransformToChildren();

    // �̺�Ʈ �߻�
    OnWorldTransformChangedDelegate.Broadcast(WorldTransform);
}

void USceneComponent::SetWorldPosition(const Vector3& InWorldPosition)
{
    // ���� ���� Ʈ�������� �ֽ� ���°� �ƴϸ� ������Ʈ
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
    // ���� ���� Ʈ�������� �ֽ� ���°� �ƴϸ� ������Ʈ
    if (bWorldTransformDirty)
    {
        UpdateWorldTransform();
    }

     // ��ȭ���� �ǹ� �ִ��� Ȯ��
    float Dot = Quaternion::Dot(Quaternion::Identity, InWorldRotation);
    float ChangeMagnitude = std::abs(1.0f - std::abs(Dot));

    if (ChangeMagnitude > TRANSFORM_EPSILON)
    {
        FTransform NewWorldTransform = WorldTransform;

         // ����ȭ �� ����
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
    // ���� ���� Ʈ�������� �ֽ� ���°� �ƴϸ� ������Ʈ
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

    // ��ȭ���� �ǹ� �ִ��� Ȯ��
    float Dot = Quaternion::Dot(Quaternion::Identity, InDeltaRotation);
    float ChangeMagnitude = std::abs(1.0f - std::abs(Dot));

    if (ChangeMagnitude > TRANSFORM_EPSILON)
    {
        FTransform NewTransform = WorldTransform;

        // ���� ȸ���� �� ȸ���� �ռ�
        XMVECTOR CurrentQuat = XMLoadFloat4(&NewTransform.Rotation);
        XMVECTOR DeltaQuat = XMLoadFloat4(&InDeltaRotation);

        // ���ʹϾ� ������ ȸ�� ���� (���� �߿�: ��Ÿ * ����)
        XMVECTOR ResultQuat = XMQuaternionMultiply(DeltaQuat, CurrentQuat);

        // ����ȭ �� ����
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

// Ʈ������ ������Ʈ �Լ�
void USceneComponent::UpdateWorldTransform() const
{
    if (!bWorldTransformDirty)
        return;

    auto Parent = GetSceneParent();
    if (Parent)
    {
        // �θ��� ���� Ʈ������ ��������
        const FTransform& ParentWorldTransform = Parent->GetWorldTransform();

        // ���ÿ��� ����� ��ȯ
        WorldTransform = LocalToWorld(ParentWorldTransform);
    }
    else
    {
        // �θ� ���� ��� ���� Ʈ�������� ���� Ʈ������
        WorldTransform = LocalTransform;
    }

    bWorldTransformDirty = false;
}

void USceneComponent::UpdateLocalTransform()
{
    auto Parent = GetSceneParent();
    if (Parent)
    {
        // �θ��� ���� Ʈ������ ��������
        const FTransform& ParentWorldTransform = Parent->GetWorldTransform();

        // ���忡�� ���÷� ��ȯ
        LocalTransform = WorldToLocal(ParentWorldTransform);
    }
    else
    {
        // �θ� ���� ��� ���� Ʈ�������� ���� Ʈ������
        LocalTransform = WorldTransform;
    }

    bLocalTransformDirty = false;

    // �̺�Ʈ �߻�
    OnLocalTransformChangedDelegate.Broadcast(LocalTransform);
}

// Ʈ������ ��ȯ �Լ�
FTransform USceneComponent::LocalToWorld(const FTransform& InParentWorldTransform) const
{
    FTransform Result;

    // ������: �θ� ������ * ���� ������
    Result.Scale = Vector3(
        InParentWorldTransform.Scale.x * LocalTransform.Scale.x,
        InParentWorldTransform.Scale.y * LocalTransform.Scale.y,
        InParentWorldTransform.Scale.z * LocalTransform.Scale.z
    );

    // ȸ��: �θ� ȸ�� * ���� ȸ��
    XMVECTOR ParentRot = XMLoadFloat4(&InParentWorldTransform.Rotation);
    XMVECTOR LocalRot = XMLoadFloat4(&LocalTransform.Rotation);
    XMVECTOR WorldRot = XMQuaternionMultiply(ParentRot, LocalRot);
    XMStoreFloat4(&Result.Rotation, WorldRot);

    // ��ġ: �θ� ��ġ + (�θ� ȸ�� * �θ� ������ * ���� ��ġ)
    XMVECTOR LocalPos = XMLoadFloat3(&LocalTransform.Position);
    XMVECTOR ParentScale = XMLoadFloat3(&InParentWorldTransform.Scale);

    // ������ ����
    XMVECTOR ScaledPos = XMVectorMultiply(LocalPos, ParentScale);

    // ȸ�� ����
    XMVECTOR RotatedPos = XMVector3Rotate(ScaledPos, ParentRot);

    // �θ� ��ġ�� ���ϱ�
    XMVECTOR ParentPos = XMLoadFloat3(&InParentWorldTransform.Position);
    XMVECTOR WorldPos = XMVectorAdd(ParentPos, RotatedPos);

    XMStoreFloat3(&Result.Position, WorldPos);

    return Result;
}

FTransform USceneComponent::WorldToLocal(const FTransform& InParentWorldTransform) const
{
    FTransform Result;

    // ������: ���� ������ / �θ� ������
    Result.Scale = Vector3(
        WorldTransform.Scale.x / InParentWorldTransform.Scale.x,
        WorldTransform.Scale.y / InParentWorldTransform.Scale.y,
        WorldTransform.Scale.z / InParentWorldTransform.Scale.z
    );

    // ȸ��: �θ� ȸ���� �� * ���� ȸ��
    XMVECTOR ParentRot = XMLoadFloat4(&InParentWorldTransform.Rotation);
    XMVECTOR WorldRot = XMLoadFloat4(&WorldTransform.Rotation);
    XMVECTOR InvParentRot = XMQuaternionInverse(ParentRot);
    XMVECTOR LocalRot = XMQuaternionMultiply(InvParentRot, WorldRot);
    XMStoreFloat4(&Result.Rotation, LocalRot);

    // ��ġ: (�θ� ȸ���� �� * (���� ��ġ - �θ� ��ġ)) / �θ� ������
    XMVECTOR WorldPos = XMLoadFloat3(&WorldTransform.Position);
    XMVECTOR ParentPos = XMLoadFloat3(&InParentWorldTransform.Position);
    XMVECTOR RelativePos = XMVectorSubtract(WorldPos, ParentPos);

    // �θ� ȸ���� �� ����
    XMVECTOR UnrotatedPos = XMVector3Rotate(RelativePos, InvParentRot);

    // �θ� �������� �� ����
    XMVECTOR InvParentScale = XMVectorReciprocal(XMLoadFloat3(&InParentWorldTransform.Scale));
    XMVECTOR LocalPos = XMVectorMultiply(UnrotatedPos, InvParentScale);

    XMStoreFloat3(&Result.Position, LocalPos);

    return Result;
}

// Ʈ������ ���� �Լ�
void USceneComponent::PropagateWorldTransformToChildren()
{
    // �ڽ� ������Ʈ�� ���� Ʈ������ ���� ����
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

// �÷��� ���� �Լ�
void USceneComponent::MarkLocalTransformDirty()
{
    bLocalTransformDirty = true;
    bWorldTransformDirty = true;

    // �̺�Ʈ �߻�
    OnLocalTransformChangedDelegate.Broadcast(LocalTransform);

    // �ڽĵ��� ���� Ʈ�������� ��Ƽ�� ǥ��
    PropagateWorldTransformToChildren();
}

void USceneComponent::MarkWorldTransformDirty()
{
    bWorldTransformDirty = true;
}

// Ʈ������ ������
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
   // ���� ���� ��ġ ��������
    Vector3 WorldPos = GetWorldPosition();

    // Ÿ�� ���� ���� ���
    Vector3 Direction = TargetWorldPosition - WorldPos;

    // ���� ���Ͱ� �ʹ� ������ ȸ������ ����
    if (Direction.LengthSquared() < TRANSFORM_EPSILON)
        return;

    // ������ ����ȭ
    Direction.Normalize();

    // �⺻������ ���� ���� ���ʹ� (0,0,1)�̰�, ���� ���ʹ� (0,1,0)
    Vector3 Forward = Vector3::Forward;
    Vector3 Up = Vector3::Up;

    // ���⿡ �´� ȸ�� ���ʹϾ� ����
    Quaternion NewRotation = Quaternion::LookRotation(Direction, Up);

    // ���� ȸ�� ����
    SetWorldRotation(NewRotation);
}

void USceneComponent::RotateAroundAxis(const Vector3& Axis, float AngleDegrees)
{
   // ������ �ʹ� �۰ų� ���� �ʹ� ������ ȸ������ ����
    if (std::abs(AngleDegrees) < TRANSFORM_EPSILON || Axis.LengthSquared() < TRANSFORM_EPSILON)
        return;

    // ���� ���� ȸ�� ��������
    Quaternion WorldRot = GetWorldRotation();

    // ȸ���� ����ȭ
    Vector3 NormalizedAxis = Axis;
    NormalizedAxis.Normalize();

    // �������� ��ȯ
    float AngleRadians = Math::DegreeToRad(AngleDegrees);

    // ȸ�� ���ʹϾ� ����
    XMVECTOR AxisVec = XMLoadFloat3(&NormalizedAxis);
    XMVECTOR DeltaRot = XMQuaternionRotationAxis(AxisVec, AngleRadians);

    // ���� ȸ���� ����
    XMVECTOR CurrentRot = XMLoadFloat4(&WorldRot);
    XMVECTOR ResultRot = XMQuaternionMultiply(DeltaRot, CurrentRot);

    // ����ȭ �� ����
    ResultRot = XMQuaternionNormalize(ResultRot);

    Quaternion NewRotation;
    XMStoreFloat4(&NewRotation, ResultRot);

    // ���� ȸ�� ����
    SetWorldRotation(NewRotation);
}

// �θ� ���� �������̵�
void USceneComponent::SetParent(const std::shared_ptr<USceneComponent>& InParent)
{
    // �θ� ���� �� ���� ���� Ʈ������ ����
    FTransform CurrentWorldTransform = GetWorldTransform();

    // ���� �θ𿡼� �и�
    auto OldParent = GetParent();
    if (OldParent)
    {
        UActorComponent::SetParent(nullptr);
    }

    // �� �θ� ������ ����
    if (InParent)
    {
        // �θ�-�ڽ� ���� ����
        UActorComponent::SetParent(InParent);

        // ���� Ʈ������ ������ ���� ���� Ʈ������ ���
        WorldTransform = CurrentWorldTransform;
        bWorldTransformDirty = false;
        UpdateLocalTransform();
    }
    else
    {
        // �θ� ���� ���, ���� = ����
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

    // ������: �θ� ������ * �ڽ� ���� ������
    Result.Scale = Vector3(
        ParentWorld.Scale.x * ChildLocal.Scale.x,
        ParentWorld.Scale.y * ChildLocal.Scale.y,
        ParentWorld.Scale.z * ChildLocal.Scale.z
    );

    // ȸ��: �θ� ȸ�� * �ڽ� ���� ȸ��
    XMVECTOR ParentRot = XMLoadFloat4(&ParentWorld.Rotation);
    XMVECTOR LocalRot = XMLoadFloat4(&ChildLocal.Rotation);
    XMVECTOR WorldRot = XMQuaternionMultiply(ParentRot, LocalRot);
    XMStoreFloat4(&Result.Rotation, WorldRot);

    // ��ġ: �θ� ��ġ + (�θ� ȸ�� * �θ� ������ * �ڽ� ���� ��ġ)
    XMVECTOR LocalPos = XMLoadFloat3(&ChildLocal.Position);
    XMVECTOR ParentScale = XMLoadFloat3(&ParentWorld.Scale);

    // ������ ����
    XMVECTOR ScaledPos = XMVectorMultiply(LocalPos, ParentScale);

    // ȸ�� ����
    XMVECTOR RotatedPos = XMVector3Rotate(ScaledPos, ParentRot);

    // �θ� ��ġ�� ���ϱ�
    XMVECTOR ParentPos = XMLoadFloat3(&ParentWorld.Position);
    XMVECTOR WorldPos = XMVectorAdd(ParentPos, RotatedPos);

    XMStoreFloat3(&Result.Position, WorldPos);

    return Result;
}

static FTransform WorldToLocal(const FTransform& ChildWorld, const FTransform& ParentWorld)
{
    FTransform Result;

    // ������: �ڽ� ���� ������ / �θ� ������
    Result.Scale = Vector3(
        ChildWorld.Scale.x / ParentWorld.Scale.x,
        ChildWorld.Scale.y / ParentWorld.Scale.y,
        ChildWorld.Scale.z / ParentWorld.Scale.z
    );

    // ȸ��: �θ� ȸ���� �� * �ڽ� ���� ȸ��
    XMVECTOR ParentRot = XMLoadFloat4(&ParentWorld.Rotation);
    XMVECTOR WorldRot = XMLoadFloat4(&ChildWorld.Rotation);
    XMVECTOR InvParentRot = XMQuaternionInverse(ParentRot);
    XMVECTOR LocalRot = XMQuaternionMultiply(InvParentRot, WorldRot);
    XMStoreFloat4(&Result.Rotation, LocalRot);

    // ��ġ: (�θ� ȸ���� �� * (�ڽ� ���� ��ġ - �θ� ��ġ)) / �θ� ������
    XMVECTOR WorldPos = XMLoadFloat3(&ChildWorld.Position);
    XMVECTOR ParentPos = XMLoadFloat3(&ParentWorld.Position);
    XMVECTOR RelativePos = XMVectorSubtract(WorldPos, ParentPos);

    // �θ� ȸ���� �� ����
    XMVECTOR UnrotatedPos = XMVector3Rotate(RelativePos, InvParentRot);

    // �θ� �������� �� ����
    XMVECTOR InvParentScale = XMVectorReciprocal(XMLoadFloat3(&ParentWorld.Scale));
    XMVECTOR LocalPos = XMVectorMultiply(UnrotatedPos, InvParentScale);

    XMStoreFloat3(&Result.Position, LocalPos);

    return Result;
}