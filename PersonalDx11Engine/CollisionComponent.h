#pragma once
#include "Math.h"
#include <memory>
#include "ObjectComponentInterface.h"
#include "Delegate.h"

struct FBoundingVolume
{
    Vector3 Center = Vector3::Zero;
    Vector3 Extents = Vector3::Zero; 

    inline float GetRadius() const { return Extents.x; };
    inline float GetHalfHeight() const { return Extents.y; };

    inline float GetMaxElementOfExtent() const
    {
        return std::max(std::max(Extents.x, Extents.y), Extents.z);
    }

    bool IsIntersectBox(const FBoundingVolume& Other) const;
    bool IsIntersectSphere(const FBoundingVolume& Other) const;
    bool IsIntersectCapsule(const FBoundingVolume& Other) const;

private: float SegmentToSegmentDistance(XMVECTOR A0, XMVECTOR A1,
                                        XMVECTOR B0, XMVECTOR B1) const;
    
};

// �浹 ���¸� �����ϴ� ������
enum class ECollisionShape
{
    Box,
    Sphere,
    Capsule,
};

// �浹 ���� Ÿ��
enum class ECollisionResponse
{
    Ignore,         // �浹 ����
    Overlap,        // ��ħ ������
    Block           // ������ �浹 ����
};

class UCollisionComponent : public IObejctCompoenent
{
public:
    UCollisionComponent(std::shared_ptr<class UGameObject> InOwner) : Owner(InOwner) 
    {};

    inline void SetCollisionShape(ECollisionShape InShape) { Shape = InShape; }
    inline void SetCollisionResponse(ECollisionResponse InResponse) { Response = InResponse; }
    inline void SetExtent(const Vector3& InExtent) { Extent = InExtent; UpdateBounds(); }

    bool CheckCollision(const UCollisionComponent* Other) const;

    FDelegate<const UCollisionComponent*> OnCollisionBegin;
    FDelegate<const UCollisionComponent*> OnCollisionEnd;

    const FBoundingVolume& GetBoudningVolume() const {return BoundingVolume; }

private:
    void UpdateBounds();

    ECollisionShape Shape = ECollisionShape::Box;
    ECollisionResponse Response = ECollisionResponse::Block;
    Vector3 Extent = Vector3(1.0f, 1.0f, 1.0f);

    FBoundingVolume BoundingVolume;

    std::weak_ptr<class UGameObject> Owner;
};
