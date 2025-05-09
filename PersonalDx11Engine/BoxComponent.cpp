#include "BoxComponent.h"
#include "DebugDrawerManager.h"
#include "Debug.h"
#include "PhysicsDefine.h"

Vector3 UBoxComponent::GetWorldSupportPoint(const Vector3& WorldDirection) const
{
    // 입력 방향 확인 및 정규화
    XMVECTOR Dir = XMLoadFloat3(&WorldDirection);
    if (XMVector3LengthSq(Dir).m128_f32[0] < KINDA_SMALL)
    {
        Dir = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    }
    Dir = XMVector3Normalize(Dir);

    // 로컬 방향으로 변환
    XMMATRIX ModelingMatrix = GetWorldTransform().GetModelingMatrix();
    XMMATRIX InvModeling = XMMatrixInverse(nullptr, ModelingMatrix);
    XMVECTOR LocalDir = XMVector3Transform(Dir, InvModeling);

    // 부호에 따라 지원점 계산
    Vector3 HalfExtent = GetHalfExtent();
    XMVECTOR SupportExtent = XMLoadFloat3(&HalfExtent);
    XMVECTOR SignMask = XMVectorGreaterOrEqual(LocalDir, XMVectorZero());
    XMVECTOR LocalSupport = XMVectorSelect(XMVectorNegate(SupportExtent), SupportExtent, SignMask);

    // 월드 공간으로 변환
    XMVECTOR WorldSupport = XMVector3TransformCoord (LocalSupport, ModelingMatrix);

    Vector3 Result;
    XMStoreFloat3(&Result, WorldSupport);
    return Result;
}

Vector3 UBoxComponent::CalculateInertiaTensor(float Mass) const
{
    // Load scaled half extents (world scale applied) into SIMD vector
    Vector3 ScaledHalfExtent = GetScaledHalfExtent(); // cm 단위
    XMVECTOR vScaledHalfExtent = XMLoadFloat3(&ScaledHalfExtent);

    // Convert cm to meters (1 cm = 0.01 m)
    vScaledHalfExtent = XMVectorScale(vScaledHalfExtent, 1.0f/ONE_METER);

    // Square each component: (he * 0.01)² = he² * 0.0001
    XMVECTOR vSquared = XMVectorMultiply(vScaledHalfExtent, vScaledHalfExtent); // (x², y², z²) in m²

    // Scale by 4 to account for full extent: (2*he)² = 4*he²
    vSquared = XMVectorScale(vSquared, 4.0f);

    // Shuffle to get the proper sum combinations for inertia tensor
    // I.x = (y² + z²)
    // I.y = (x² + z²)
    // I.z = (x² + y²)
    XMVECTOR tensor = XMVectorSet(
        XMVectorGetY(vSquared) + XMVectorGetZ(vSquared), // y² + z²
        XMVectorGetX(vSquared) + XMVectorGetZ(vSquared), // x² + z²
        XMVectorGetX(vSquared) + XMVectorGetY(vSquared), // x² + y²
        0.0f
    );

    // Scale with mass * (1/12)
    float Scale = Mass / 12.0f;
    XMVECTOR vInerteria = XMVectorScale(tensor, Scale);

    // Store result
    Vector3 Result;
    XMStoreFloat3(&Result, vInerteria);
    return Result;
}

void UBoxComponent::CalculateAABB(Vector3& OutMin, Vector3& OutMax) const
{
    // Load world transform matrix (Scale * Rotation * Translation)
    XMMATRIX WorldTransformMatrix = GetWorldTransform().GetModelingMatrix();

    // Preload GetHalfExtent() for local box
    float hx = GetHalfExtent().x;
    float hy = GetHalfExtent().y;
    float hz = GetHalfExtent().z;

    // Precomputed 8 local corners in SIMD directly
    XMVECTOR Points[8];
    Points[0] = XMVectorSet(-hx, -hy, -hz, 1.0f); // w=1 for full transform
    Points[1] = XMVectorSet(+hx, -hy, -hz, 1.0f);
    Points[2] = XMVectorSet(-hx, +hy, -hz, 1.0f);
    Points[3] = XMVectorSet(+hx, +hy, -hz, 1.0f);
    Points[4] = XMVectorSet(-hx, -hy, +hz, 1.0f);
    Points[5] = XMVectorSet(+hx, -hy, +hz, 1.0f);
    Points[6] = XMVectorSet(-hx, +hy, +hz, 1.0f);
    Points[7] = XMVectorSet(+hx, +hy, +hz, 1.0f);

    // Apply full world transform (Scale, Rotation, Translation)
    for (int i = 0; i < 8; ++i)
        Points[i] = XMVector4Transform(Points[i], WorldTransformMatrix);

    // Initialize Min/Max with first transformed point
    XMVECTOR MinPoint = Points[0];
    XMVECTOR MaxPoint = Points[0];

    for (int i = 1; i < 8; ++i)
    {
        MinPoint = XMVectorMin(MinPoint, Points[i]);
        MaxPoint = XMVectorMax(MaxPoint, Points[i]);
    }

    // Store back (only x, y, z components)
    XMStoreFloat3(&OutMin, MinPoint);
    XMStoreFloat3(&OutMax, MaxPoint);
}

void UBoxComponent::RequestDebugRender(const float DeltaTime)
{
    
    //static float accum = 0.0f;
    //static const float duration = 1.0f;
    //accum += DeltaTime;
    //if (accum > duration)
    //{
    //    auto& WolrdTransform = GetWorldTransform();
    //    auto& LocalTransform = GetLocalTransform();
    //    accum = 0.0f;
    //    LOG("BOX WolrdTransform ==========\n %s \n===============",Debug::ToString(WolrdTransform));
    //    LOG("BOX LocalTransform ==========\n %s \n===============",Debug::ToString(LocalTransform));
    //}
    UDebugDrawManager::Get()->DrawBox(
        GetWorldPosition(),
        GetWorldScale(),
        GetWorldRotation(),
        Vector4(1, 1, 0, 1),
        DeltaTime
    );
}
