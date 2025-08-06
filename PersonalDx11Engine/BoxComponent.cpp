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
    Vector3 HalfExtent = GetLocalHalfExtent();
    XMVECTOR SupportExtent = XMLoadFloat3(&HalfExtent);
    XMVECTOR SignMask = XMVectorGreaterOrEqual(LocalDir, XMVectorZero());
    XMVECTOR LocalSupport = XMVectorSelect(XMVectorNegate(SupportExtent), SupportExtent, SignMask);

    // 월드 공간으로 변환
    XMVECTOR WorldSupport = XMVector3TransformCoord (LocalSupport, ModelingMatrix);

    Vector3 Result;
    XMStoreFloat3(&Result, WorldSupport);
    return Result;
}

Vector3 UBoxComponent::CalculateInvInertiaTensor(float InvMass) const
{
    //무한대 질량(Static Body)인 경우
    if (InvMass < KINDA_SMALL)
    {
        return Vector3::Zero();
    }

    // Load scaled half extents (world scale applied) into SIMD vector
    Vector3 ScaledHalfExtent = GetScaledHalfExtent(); 
    Vector3 Size = ScaledHalfExtent * 2.0f;

    // 박스 관성텐서 공식: I = (m/12) * (h²+d², w²+d², w²+h²)
    float SizeX2 = Size.x * Size.x;
    float SizeY2 = Size.y * Size.y;
    float SizeZ2 = Size.z * Size.z;

    Vector3 InertiaTensor;
    InertiaTensor.x = (SizeY2 + SizeZ2) / 12.0f;  // Ixx
    InertiaTensor.y = (SizeX2 + SizeZ2) / 12.0f;  // Iyy
    InertiaTensor.z = (SizeX2 + SizeY2) / 12.0f;  // Izz

    // 역관성텐서 = InvMass / InertiaTensor
    Vector3 InvInertiaTensor;
    InvInertiaTensor.x = (InertiaTensor.x > KINDA_SMALL) ? InvMass / InertiaTensor.x : 0.0f;
    InvInertiaTensor.y = (InertiaTensor.y > KINDA_SMALL) ? InvMass / InertiaTensor.y : 0.0f;
    InvInertiaTensor.z = (InertiaTensor.z > KINDA_SMALL) ? InvMass / InertiaTensor.z : 0.0f;

    return InvInertiaTensor;
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
