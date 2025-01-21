#include "Transform.h"

Matrix FTransform::GetModelingMatrix() const
{
    // XMFLOAT3를 XMVECTOR로 변환
    DirectX::XMVECTOR VPosition = DirectX::XMLoadFloat3(&Position);
    DirectX::XMVECTOR VRotation = DirectX::XMLoadFloat3(&Rotation);
    DirectX::XMVECTOR VScale = DirectX::XMLoadFloat3(&Scale);

    //매트릭스 생성
    DirectX::XMMATRIX ScaleMatrix = DirectX::XMMatrixScalingFromVector(VScale);
    DirectX::XMMATRIX RotationMatrix = DirectX::XMMatrixRotationRollPitchYawFromVector(VRotation);
    DirectX::XMMATRIX TranslationMatrix = DirectX::XMMatrixTranslationFromVector(VPosition);

    // M = SRT
    return ScaleMatrix * RotationMatrix * TranslationMatrix;
}
