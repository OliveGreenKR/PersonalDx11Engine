#include "SphereComponent.h"
#include "DebugDrawerManager.h"

Vector3 USphereComponent::GetLocalSupportPoint(const Vector3& WorldDirection) const
{
    // 입력 방향 확인 및 정규화
    XMVECTOR Dir = XMLoadFloat3(&WorldDirection);
    if (XMVector3LengthSq(Dir).m128_f32[0] < KINDA_SMALL)
    {
        Dir = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    }
    Dir = XMVector3Normalize(Dir);

    // 월드 → 로컬 방향 변환
    XMMATRIX ModelingMatrix = GetWorldTransform().GetModelingMatrix();
    XMMATRIX InvModeling = XMMatrixInverse(nullptr, ModelingMatrix);
    XMVECTOR LocalDir = XMVector3Transform(Dir, InvModeling);

    // 0 벡터 방지
    if (XMVector3LengthSq(LocalDir).m128_f32[0] < KINDA_SMALL)
    {
        LocalDir = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f); // 기본 Y 방향
    }

    LocalDir = XMVector3Normalize(LocalDir);

    // 로컬 구의 중심은 항상 (0,0,0), 반지름은 x축 기준 반지름
    float Radius = GetScaledHalfExtent().x;
    XMVECTOR SupportLocal = XMVectorScale(LocalDir, Radius);

    Vector3 Result;
    XMStoreFloat3(&Result, SupportLocal);
    return Result;
}

Vector3 USphereComponent::CalculateInertiaTensor(float Mass) const
{
    // For a solid sphere: I = (2/5) * m * r^2
    float r = GetHalfExtent().x;
    float r2 = r * r;
    float Inertia = (2.0f / 5.0f) * Mass * r2;

    return Vector3(Inertia, Inertia, Inertia);
}

void USphereComponent::CalculateAABB(Vector3& OutMin, Vector3& OutMax) const
{
    XMVECTOR Center = XMLoadFloat3(&GetWorldTransform().Position);
    XMVECTOR Extent = XMVectorReplicate(GetHalfExtent().x); // (r, r, r, r)

    XMVECTOR MinPoint = XMVectorSubtract(Center, Extent);
    XMVECTOR MaxPoint = XMVectorAdd(Center, Extent);

    XMStoreFloat3(&OutMin, MinPoint);
    XMStoreFloat3(&OutMax, MaxPoint);
}

void USphereComponent::RequestDebugRender(const float DeltaTime)
{
    UDebugDrawManager::Get()->DrawSphere(
        GetWorldPosition(),
        GetWorldScale().x * 0.5f,
        GetWorldRotation(),
        Vector4(1, 1, 0, 1),
        DeltaTime
    );
}
