#include "Transform.h"


void FTransform::RotateAroundAxis(const Vector3& InAxis, float AngleDegrees)
{
    if (AngleDegrees <= KINDA_SMALL || InAxis.LengthSquared() < KINDA_SMALL)
        return;

    // 현재 회전 행렬
    Matrix CurrentRotation = GetRotationMatrix();

    // 월드 공간 회전축을 로컬 공간으로 변환
    XMVECTOR LocalAxis = XMVector3TransformNormal(XMLoadFloat3(&InAxis), CurrentRotation);

    //회전축 정규화
    LocalAxis = XMVector3Normalize(LocalAxis);

    // 회전 쿼터니온 생성
    float AngleRadians = Math::DegreeToRad(AngleDegrees);
    XMVECTOR DeltaRotation = XMQuaternionRotationAxis(LocalAxis, AngleRadians);

    // 기존 회전에 새 회전 결합
    XMVECTOR CurrentQuat = XMLoadFloat4(&Rotation);
    XMVECTOR FinalQuat = XMQuaternionMultiply(DeltaRotation, CurrentQuat);

    // 정규화 및 저장
    FinalQuat = XMQuaternionNormalize(FinalQuat);
    XMStoreFloat4(&Rotation, FinalQuat);
}

FTransform FTransform::Lerp(const FTransform& Start, const FTransform& End, float Alpha)
{
    FTransform Result;

    // 위치 선형 보간
    Result.Position = Math::Lerp(Start.Position, End.Position, Alpha);

    // 회전 구면 선형 보간
    Result.Rotation = Math::Slerp(Start.Rotation, End.Rotation, Alpha);

    // 스케일 선형 보간
    Result.Scale = Math::Lerp(Start.Scale, End.Scale, Alpha);

    return Result;
}

Matrix FTransform::GetTranslationMatrix() const
{
    return DirectX::XMMatrixTranslation(Position.x, Position.y, Position.z);
}

Matrix FTransform::GetScaleMatrix() const
{
    return DirectX::XMMatrixScaling(Scale.x, Scale.y, Scale.z);
}

Matrix FTransform::GetRotationMatrix() const
{
    DirectX::XMVECTOR VRotation = DirectX::XMLoadFloat4(&Rotation);
    VRotation = DirectX::XMQuaternionNormalize(VRotation);
    return DirectX::XMMatrixRotationQuaternion(VRotation);
}

Matrix FTransform::GetModelingMatrix() const
{
    //매트릭스 생성
    Matrix ScaleMatrix = GetScaleMatrix();
    Matrix RotationMatrix = GetRotationMatrix();
    Matrix TranslationMatrix = GetTranslationMatrix();;

    // M = S*R*T
    return ScaleMatrix * RotationMatrix * TranslationMatrix;
}
