#include "Transform.h"

void FTransform::SetRotation(const Vector3& InEulerAngles)
{
    Rotation = Math::EulerToQuaternion(InEulerAngles);
}

void FTransform::SetRotation(const Quaternion& InQuaternion)
{
    float lengthSquared = InQuaternion.LengthSquared();
    if (std::abs(lengthSquared - 1.0f) > KINDA_SMALL)
    {
        Rotation = InQuaternion.GetNormalized();
        return;
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

void FTransform::AddRotation(const Quaternion& InQuaternion)
{
    Matrix Current = XMMatrixRotationQuaternion(XMLoadFloat4(&Rotation));
    Matrix Delta = XMMatrixRotationQuaternion(XMLoadFloat4(&InQuaternion));
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

    // 축-각도 회전 행렬 생성
    XMMATRIX Delta = XMMatrixRotationAxis(NormalizedAxis, AngleRadians);

    // 행렬 곱셈으로 회전 결합
    XMMATRIX Final = Current * Delta;

    // 최종 행렬을 쿼터니온으로 변환
    XMStoreFloat4(&Rotation, XMQuaternionRotationMatrix(Final));

    // 수치 안정성을 위한 정규화
    Rotation.Normalize();
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
    //매트릭스 생성
    Matrix ScaleMatrix = GetScaleMatrix();
    Matrix RotationMatrix = GetRotationMatrix();
    Matrix TranslationMatrix = GetTranslationMatrix();;

    // M = SRT
    return ScaleMatrix * RotationMatrix * TranslationMatrix;
}
