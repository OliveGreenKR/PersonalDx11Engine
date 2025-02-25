#pragma once
#include "GameObject.h"
#include "Color.h"
#include "Model.h"
#include <random>

enum class ECollisionShapeType;


class UElasticBody : public UGameObject
{
public:
    enum class EShape
    {
        Box,
        Sphere
    };

    UElasticBody();
    virtual ~UElasticBody() = default;

    // 기본 수명 주기 메서드
    virtual void Tick(const float DeltaTime) override;
    virtual void PostInitialized() override;
    virtual void PostInitializedComponents() override;

    // 컴포넌트 접근자
    class URigidBodyComponent* GetRigidBody() const { return Rigid.lock().get(); }
    class UCollisionComponent* GetCollisionComponent() const { return Collision.lock().get(); }

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


    // 활성화/비활성화
    void SetActive(bool bActive);
    bool IsActive() const { return bIsActive; }

    // 외부 접근 방지를 위한 friend 선언
    friend class UElasticBodyManager;

private:
    bool bIsActive = true;
    EShape Shape = EShape::Sphere;

    // 컴포넌트 참조
    std::weak_ptr<class URigidBodyComponent> Rigid;
    std::weak_ptr<class UCollisionComponent> Collision;

    // 내부 유틸리티 메서드
    ECollisionShapeType GetCollisionShape(EShape Shape);
};