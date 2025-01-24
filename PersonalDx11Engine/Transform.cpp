#include "Transform.h"

void FTransform::SetRotation(const Vector3& InEulerAngles)
{
    Rotation = Math::EulerToQuaternion(InEulerAngles);
}

void FTransform::SetRotation(Quaternion& InQuaternion)
{
    float lengthSquared = InQuaternion.LengthSquared();
    if (std::abs(lengthSquared - 1.0f) > KINDA_SMALL)
    {
        InQuaternion.Normalize();
    }
    Rotation = InQuaternion;
}

const Vector3 FTransform::GetEulerRotation() const
{
    return Math::QuaternionToEuler(Rotation);
}

const Quaternion FTransform::GetQuarternionRotation() const
{
    return Rotation;
}

void FTransform::AddRotation(const Vector3& InEulerAngles)
{
    Vector3 RadAngles = InEulerAngles * (PI / 180.0f);
    //multiply mustbo right to left
    Matrix Current = XMMatrixRotationQuaternion(XMLoadFloat4(&Rotation));
    Matrix Delta = XMMatrixRotationRollPitchYaw(RadAngles.x, RadAngles.y, RadAngles.z);
    XMMATRIX Final = Current * Delta;
    XMStoreFloat4(&Rotation, XMQuaternionRotationMatrix(Final));
    Rotation.Normalize();
}

void FTransform::RotateAroundAxis(const Vector3& InAxis, float AngleDegrees)
{
    //check Axis
    if (InAxis.LengthSquared() < KINDA_SMALL)
    {
        return;
    }

    XMVECTOR NormalizedAxis = XMVector3Normalize(XMLoadFloat3(&InAxis));
    float AngleRadians = Math::DegreeToRad(AngleDegrees);
    XMMATRIX Current = XMMatrixRotationQuaternion(XMLoadFloat4(&Rotation));

    // ��-���� ȸ�� ��� ����
    XMMATRIX Delta = XMMatrixRotationAxis(NormalizedAxis, AngleRadians);

    // ��� �������� ȸ�� ����
    XMMATRIX Final = Current * Delta;

    // ���� ����� ���ʹϿ����� ��ȯ
    XMStoreFloat4(&Rotation, XMQuaternionRotationMatrix(Final));

    // ��ġ �������� ���� ����ȭ
    Rotation.Normalize();
}

Matrix FTransform::GetModelingMatrix() const
{
    // XMFLOAT3�� XMVECTOR�� ��ȯ
    DirectX::XMVECTOR VPosition = DirectX::XMLoadFloat3(&Position);
    DirectX::XMVECTOR VRotation = DirectX::XMLoadFloat4(&Rotation);
    DirectX::XMVECTOR VScale = DirectX::XMLoadFloat3(&Scale);

    //��Ʈ���� ����
    DirectX::XMMATRIX ScaleMatrix = DirectX::XMMatrixScalingFromVector(VScale);
    DirectX::XMMATRIX RotationMatrix = XMMatrixRotationQuaternion(VRotation);
    DirectX::XMMATRIX TranslationMatrix = DirectX::XMMatrixTranslationFromVector(VPosition);

    // M = SRT
    return ScaleMatrix * RotationMatrix * TranslationMatrix;
}
