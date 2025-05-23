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

bool FTransform::IsEqual(const FTransform& WorldTransform , const FTransform& InWorldTransform, const float Epsilon)
{
    bool bIsSame = (
        (WorldTransform.Position - InWorldTransform.Position).LengthSquared() < TRANSFORM_EPSILON * TRANSFORM_EPSILON) &&
        ((std::abs(1.0f - std::abs(Quaternion::Dot(WorldTransform.Rotation, InWorldTransform.Rotation)))) < TRANSFORM_EPSILON) &&
        ((WorldTransform.Scale - InWorldTransform.Scale).LengthSquared() < TRANSFORM_EPSILON * TRANSFORM_EPSILON);
    
    return bIsSame;
}

bool FTransform::IsValidPosition(const Vector3& InVector)
{
    return InVector.LengthSquared() > TRANSFORM_EPSILON * TRANSFORM_EPSILON;
}

bool FTransform::IsValidScale(const Vector3& InVector)
{
    return InVector.LengthSquared() > TRANSFORM_EPSILON * TRANSFORM_EPSILON;
}

bool FTransform::IsValidRotation(const Quaternion & QuatA, const Quaternion & QuatB)
{
    // 변화량이 의미 있는지 확인
    float Dot = Quaternion::Dot(QuatA, QuatB);
    float ChangeMagnitude = std::abs(1.0f - std::abs(Dot));

    return ChangeMagnitude > TRANSFORM_EPSILON;
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
