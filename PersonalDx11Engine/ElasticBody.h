#pragma once
#include "GameObject.h"
#include "Color.h"
#include "Model.h"
#include <random>
enum class ECollisionShapeType;

class UElasticBody : public UGameObject
{
    friend class UElasticBodyManager;

public:
    enum class EShape
    {
        Box,
        Sphere,
        Count
    };

    UElasticBody();
    virtual ~UElasticBody() = default;

    // 기본 수명 주기 메서드
    virtual void Tick(const float DeltaTime) override;
    virtual void PostInitialized() override;
    virtual void PostInitializedComponents() override;

    /// <summary>
    /// 인스턴스 상태 초기화
    /// </summary>
    virtual void Reset();

    // 충돌 이벤트 핸들러
    //void OnCollisionBegin(const FCollisionEventData& InCollision);
    //void OnCollisionStay(const FCollisionEventData& InCollision);
    //void OnCollisionEnd(const FCollisionEventData& InCollision);

    // Getters  
    const Vector3& GetVelocity() const;
    const Vector3& GetAngularVelocity() const;
    float GetSpeed() const;
    float GetMass() const;
    Vector3 GetRotationalInertia() const;
    float GetRestitution() const;
    float GetFrictionKinetic() const;
    float GetFrictionStatic() const;

    // 물리 속성 설정  
    void SetMass(float InMass);
    void SetMaxSpeed(float InSpeed);
    void SetMaxAngularSpeed(float InSpeed);
    void SetGravityScale(float InScale);
    void SetFrictionKinetic(float InFriction);
    void SetFrictionStatic(float InFriction);
    void SetRestitution(float InRestitution);

    //충돌체 설정
    void SetShape(EShape InShape);
    void SetShapeSphere();
    void SetShapeBox();
    void SyncCollisionShape();

    // 활성화/비활성화
    void SetActive(const bool bActive);
    bool IsActive() const { return bIsActive; }

private:
    // 내부 유틸리티 메서드
    enum class ECollisionShapeType GetCollisionShape(const EShape Shape) const;

private:
    bool bIsActive = true;
    EShape Shape = EShape::Sphere;

    // 컴포넌트 소유
    std::shared_ptr<class URigidBodyComponent> Rigid;
    std::shared_ptr<class UCollisionComponent> Collision;

public:
    class URigidBodyComponent* GetRigid();
    class UCollisionComponent* GetCollision();

};