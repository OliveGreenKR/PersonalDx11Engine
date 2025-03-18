#include "SceneComponent.h"


// ���� Ʈ������ ���� �� ȣ��Ǵ� �Լ�
void USceneComponent::OnLocalTransformChanged()
{
    // 1. ���� ���� Ʈ������ ������Ʈ
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

    // 2. ���� �̺�Ʈ �߻�
    OnLocalTransformChangedDelegate.Broadcast(LocalTransform);
    OnWorldTransformChangedDelegate.Broadcast(WorldTransform);

    // 3. �ڽ� ������Ʈ���� ���� Ʈ������ ����� ������Ʈ
    PropagateWorldTransformToChildren();
}

// ���� Ʈ������ ���� �� ȣ��Ǵ� �Լ�
void USceneComponent::OnWorldTransformChanged()
{
    // 1. ���� ���� Ʈ������ ������Ʈ
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

    // 2. ���� �̺�Ʈ �߻�
    OnLocalTransformChangedDelegate.Broadcast(LocalTransform);
    OnWorldTransformChangedDelegate.Broadcast(WorldTransform);

    // 3. �ڽ� ������Ʈ���� ���� Ʈ������ ����� ������Ʈ
    PropagateWorldTransformToChildren();
}

// �θ� ���� �� ȣ��Ǵ� �Լ�
void USceneComponent::OnChangeParent(const std::shared_ptr<USceneComponent>& NewParent)
{
    // ���� ���� Ʈ������ ����
    FTransform CurrentWorldTransform = GetWorldTransform();

    // 1. ActorComponent �θ�-�ڽ� ���� ����
    if (auto OldParent = GetParent())
    {
        UActorComponent::SetParent(nullptr);
    }

    // 2. �� �θ���� ���� ����
    if (NewParent)
    {
        UActorComponent::SetParent(NewParent);

        // 3. ���� Ʈ������ �����ϸ� ���� Ʈ������ ����
        WorldTransform = CurrentWorldTransform;
        const FTransform& ParentWorldTransform = NewParent->GetWorldTransform();
        LocalTransform = WorldToLocal(ParentWorldTransform);
    }
    else
    {
        // �θ� ���� ��� ����=����
        LocalTransform = CurrentWorldTransform;
        WorldTransform = CurrentWorldTransform;
    }

    // 4. ���� �̺�Ʈ �߻�
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
        OnLocalTransformChanged();
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

    // ���� �θ𿡼� �и� - ���θ����������� ActorComponent�� ó��
    auto OldParent = GetParent();
    //if (OldParent)
    //{
    //    DetachFromParent();
    //}

    // �� �θ� ������ ����
    if (InParent)
    {
        // �θ�-�ڽ� ���� ����
        UActorComponent::SetParent(InParent);

        // ���� Ʈ������ ������ ���� ���� Ʈ������ ���
        FTransform NewLocal = WorldToLocal(InParent->GetWorldTransform());
        SetLocalTransform(NewLocal);
    }
    else
    {
        // �θ� ���� ���, ����
        UActorComponent::SetParent(nullptr);
        LocalTransform = FTransform(); //�ʱ�ȭ(0)
        WorldTransform = CurrentWorldTransform;
    }
}