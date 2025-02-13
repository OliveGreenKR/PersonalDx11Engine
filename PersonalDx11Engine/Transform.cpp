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
        XMStoreFloat4(&Rotation, ResultV);
        NotifyTransformChanged();
    }
}

void FTransform::RotateAroundAxis(const Vector3& InAxis, float AngleDegrees)
{
    // 축 벡터 검사
    if (InAxis.LengthSquared() < KINDA_SMALL || std::abs(AngleDegrees) < KINDA_SMALL)
        return;

    // 현재 회전 로드
    XMVECTOR CurrentRotation = XMLoadFloat4(&Rotation);

    // 축 정규화 및 각도를 라디안으로 변환
    XMVECTOR NormalizedAxis = XMVector3Normalize(XMLoadFloat3(&InAxis));
    float AngleRadians = XMConvertToRadians(AngleDegrees);

    // 축과 각도로부터 회전 쿼터니온 생성
    XMVECTOR DeltaRotation = XMQuaternionRotationNormal(NormalizedAxis, AngleRadians);

    // 현재 회전에 새로운 회전 결합
    XMVECTOR ResultRotation = XMQuaternionMultiply(CurrentRotation, DeltaRotation);

    // 정규화 및 저장
    ResultRotation = XMQuaternionNormalize(ResultRotation);
    XMStoreFloat4(&Rotation, ResultRotation);

    NotifyTransformChanged();
}

FTransform FTransform::InterpolateTransform(const FTransform& Start, const FTransform& End, float Alpha)
{
    FTransform Result;

    // 위치 선형 보간
    Result.Position = Vector3::Lerp(Start.Position, End.Position, Alpha);

    // 회전 구면 선형 보간
    Result.Rotation = Vector4::Slerp(Start.Rotation, End.Rotation, Alpha);

    // 스케일 선형 보간
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
    //매트릭스 생성
    Matrix ScaleMatrix = GetScaleMatrix();
    Matrix RotationMatrix = GetRotationMatrix();
    Matrix TranslationMatrix = GetTranslationMatrix();;

    // M = SRT
    return ScaleMatrix * RotationMatrix * TranslationMatrix;
}
