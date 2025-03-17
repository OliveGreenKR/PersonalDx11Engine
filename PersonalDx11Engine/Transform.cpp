#include "Transform.h"


void FTransform::RotateAroundAxis(const Vector3& InAxis, float AngleDegrees)
{
    if (AngleDegrees <= KINDA_SMALL || InAxis.LengthSquared() < KINDA_SMALL)
        return;

    // ���� ȸ�� ���
    Matrix CurrentRotation = GetRotationMatrix();

    // ���� ���� ȸ������ ���� �������� ��ȯ
    XMVECTOR LocalAxis = XMVector3TransformNormal(XMLoadFloat3(&InAxis), CurrentRotation);

    //ȸ���� ����ȭ
    LocalAxis = XMVector3Normalize(LocalAxis);

    // ȸ�� ���ʹϿ� ����
    float AngleRadians = Math::DegreeToRad(AngleDegrees);
    XMVECTOR DeltaRotation = XMQuaternionRotationAxis(LocalAxis, AngleRadians);

    // ���� ȸ���� �� ȸ�� ����
    XMVECTOR CurrentQuat = XMLoadFloat4(&Rotation);
    XMVECTOR FinalQuat = XMQuaternionMultiply(DeltaRotation, CurrentQuat);

    // ����ȭ �� ����
    FinalQuat = XMQuaternionNormalize(FinalQuat);
    XMStoreFloat4(&Rotation, FinalQuat);
}

FTransform FTransform::InterpolateTransform(const FTransform& Start, const FTransform& End, float Alpha)
{
    FTransform Result;

    // ��ġ ���� ����
    Result.Position = Math::Lerp(Start.Position, End.Position, Alpha);

    // ȸ�� ���� ���� ����
    Result.Rotation = Math::Slerp(Start.Rotation, End.Rotation, Alpha);

    // ������ ���� ����
    Result.Scale = Math::Lerp(Start.Scale, End.Scale, Alpha);

    return Result;
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
