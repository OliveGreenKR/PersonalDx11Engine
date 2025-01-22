#include "Transform.h"

Matrix FTransform::GetModelingMatrix() const
{
    // XMFLOAT3�� XMVECTOR�� ��ȯ
    DirectX::XMVECTOR VPosition = DirectX::XMLoadFloat3(&Position);
    DirectX::XMVECTOR VRotation = DirectX::XMLoadFloat3(&Rotation);
    DirectX::XMVECTOR VScale = DirectX::XMLoadFloat3(&Scale);

    //Quarternion
    XMVECTOR Quaternion = XMQuaternionRotationRollPitchYawFromVector(VRotation);
    Quaternion = XMQuaternionNormalize(Quaternion);

    //��Ʈ���� ����
    DirectX::XMMATRIX ScaleMatrix = DirectX::XMMatrixScalingFromVector(VScale);
    DirectX::XMMATRIX RotationMatrix = XMMatrixRotationQuaternion(Quaternion);
    DirectX::XMMATRIX TranslationMatrix = DirectX::XMMatrixTranslationFromVector(VPosition);

    // M = SRT
    return ScaleMatrix * RotationMatrix * TranslationMatrix;
}
