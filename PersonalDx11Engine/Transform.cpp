#include "Transform.h"

void FTransform::SetEulerRotation(const Vector3& EulerAngles) {
    SetRotation(Math::EulerToQuaternion(EulerAngles));
}

void FTransform::SetRotation(const Quaternion& InQuaternion)
{
    // 회전 변화량 계산을 위한 내적
    float Dot = Quaternion::Dot(Rotation, InQuaternion);
    float ChangeMagnitude = std::abs(1.0f - std::abs(Dot));

    if (ChangeMagnitude > RotationThreshold)
    {
        float LengthSq = InQuaternion.LengthSquared();
        //정규화 유지
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

void FTransform::AddPosition(const Vector3& InPosition)
{
    if (InPosition.LengthSquared() > PositionThreshold * PositionThreshold)
    {
        Position += InPosition;
        NotifyTransformChanged();
    }
}

void FTransform::AddEulerRotation(const Vector3& InEulerAngles)
{
    AddRotation(Math::EulerToQuaternion(InEulerAngles));
}

void FTransform::AddRotation(const Quaternion& InQuaternion)
{
    XMVECTOR CurrentV = XMLoadFloat4(&Rotation);
    XMVECTOR DeltaV = XMLoadFloat4(&InQuaternion);

    float Dot = XMVectorGetX(XMVector4Dot(CurrentV, DeltaV));
    float ChangeMagnitude = std::abs(1.0f - std::abs(Dot));

    if (ChangeMagnitude > RotationThreshold)
    {
        // 직접 쿼터니온 곱셈으로 회전 결합
        XMVECTOR ResultV = XMQuaternionMultiply(CurrentV, DeltaV);
        ResultV = XMQuaternionNormalize(ResultV);
        Quaternion InQuaternion;
        XMStoreFloat4(&InQuaternion, ResultV);
        Rotation = InQuaternion;
        NotifyTransformChanged();
    }
}

void FTransform::RotateAroundAxis(const Vector3& InAxis, float AngleDegrees)
{
    if (InAxis.LengthSquared() < KINDA_SMALL)
        return;

    // 현재 회전 행렬
    Matrix CurrentRotation = GetRotationMatrix();

    // 월드 공간 회전축을 로컬 공간으로 변환
    XMVECTOR LocalAxis = XMVector3TransformNormal(XMLoadFloat3(&InAxis), CurrentRotation);
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

FTransform FTransform::InterpolateTransform(const FTransform& Start, const FTransform& End, float Alpha)
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

void FTransform::NotifyTransformChanged()
{
    ++TransformVersion;
    OnTransformChangedDelegate.Broadcast(*this);
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
