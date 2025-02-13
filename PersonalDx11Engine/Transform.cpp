#include "Transform.h"

void FTransform::SetEulerRotation(const Vector3& EulerAngles) {
    SetRotation(Math::EulerToQuaternion(EulerAngles));
}

void FTransform::SetRotation(const Quaternion& InQuaternion)
{
    // ȸ�� ��ȭ�� ����� ���� ����
    float Dot = Quaternion::Dot(Rotation, InQuaternion);
    float ChangeMagnitude = std::abs(1.0f - std::abs(Dot));

    if (ChangeMagnitude > RotationThreshold)
    {
        float LengthSq = InQuaternion.LengthSquared();
        //����ȭ ����
        if (std::abs(LengthSq - 1.0f) > KINDA_SMALL)
        {
            Rotation = InQuaternion.GetNormalized();
        }
        else
        {
            Rotation = InQuaternion;
        }
        NotifyTransformChanged();
    }
}

void FTransform::SetPosition(const Vector3& InPosition)
{
    if ((Position - InPosition).LengthSquared() > PositionThreshold * PositionThreshold)
    {
        Position = InPosition;
        NotifyTransformChanged();
    }
        
}

void FTransform::SetScale(const Vector3& InScale) {
    if ((InScale - Scale).LengthSquared() > ScaleThreshold * ScaleThreshold)
    {
        Scale = InScale;
        NotifyTransformChanged();
    }
}

void FTransform::AddRotation(const Quaternion& InQuaternion)
{
    XMVECTOR CurrentV = XMLoadFloat4(&Rotation);
    XMVECTOR DeltaV = XMLoadFloat4(&InQuaternion);

    float Dot = XMVectorGetX(XMVector4Dot(CurrentV, DeltaV));
    float ChangeMagnitude = std::abs(1.0f - std::abs(Dot));

    if (ChangeMagnitude > RotationThreshold)
    {
        // ���� ���ʹϿ� �������� ȸ�� ����
        XMVECTOR ResultV = XMQuaternionMultiply(CurrentV, DeltaV);
        ResultV = XMQuaternionNormalize(ResultV);
        XMStoreFloat4(&Rotation, ResultV);
        NotifyTransformChanged();
    }
}

void FTransform::RotateAroundAxis(const Vector3& InAxis, float AngleDegrees)
{
    // �� ���� �˻�
    if (InAxis.LengthSquared() < KINDA_SMALL || std::abs(AngleDegrees) < KINDA_SMALL)
        return;

    // ���� ȸ�� �ε�
    XMVECTOR CurrentRotation = XMLoadFloat4(&Rotation);

    // �� ����ȭ �� ������ �������� ��ȯ
    XMVECTOR NormalizedAxis = XMVector3Normalize(XMLoadFloat3(&InAxis));
    float AngleRadians = XMConvertToRadians(AngleDegrees);

    // ��� �����κ��� ȸ�� ���ʹϿ� ����
    XMVECTOR DeltaRotation = XMQuaternionRotationNormal(NormalizedAxis, AngleRadians);

    // ���� ȸ���� ���ο� ȸ�� ����
    XMVECTOR ResultRotation = XMQuaternionMultiply(CurrentRotation, DeltaRotation);

    // ����ȭ �� ����
    ResultRotation = XMQuaternionNormalize(ResultRotation);
    XMStoreFloat4(&Rotation, ResultRotation);

    NotifyTransformChanged();
}

FTransform FTransform::InterpolateTransform(const FTransform& Start, const FTransform& End, float Alpha)
{
    FTransform Result;

    // ��ġ ���� ����
    Result.Position = Vector3::Lerp(Start.Position, End.Position, Alpha);

    // ȸ�� ���� ���� ����
    Result.Rotation = Vector4::Slerp(Start.Rotation, End.Rotation, Alpha);

    // ������ ���� ����
    Result.Scale = Vector3::Lerp(Start.Scale, End.Scale, Alpha);

    return Result;
}

void FTransform::NotifyTransformChanged()
{
    ++TransformVersion;
    OnTransformChanged.Broadcast(*this);
}

Matrix FTransform::GetTranslationMatrix() const
{
    DirectX::XMVECTOR VPosition = DirectX::XMLoadFloat3(&Position);
    return DirectX::XMMatrixTranslationFromVector(VPosition);
}

Matrix FTransform::GetScaleMatrix() const
{
    DirectX::XMVECTOR VScale = DirectX::XMLoadFloat3(&Scale);
    return XMMatrixScalingFromVector(VScale);
}

Matrix FTransform::GetRotationMatrix() const
{
    DirectX::XMVECTOR VRotation = DirectX::XMLoadFloat4(&Rotation);
    return XMMatrixRotationQuaternion(VRotation);
}

Matrix FTransform::GetModelingMatrix() const
{
    //��Ʈ���� ����
    Matrix ScaleMatrix = GetScaleMatrix();
    Matrix RotationMatrix = GetRotationMatrix();
    Matrix TranslationMatrix = GetTranslationMatrix();;

    // M = SRT
    return ScaleMatrix * RotationMatrix * TranslationMatrix;
}
