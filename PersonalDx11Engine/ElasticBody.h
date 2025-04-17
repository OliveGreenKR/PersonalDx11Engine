#pragma once
#include "GameObject.h"
#include "Model.h"
#include <random>

enum class EElasticBodyShape
{
    Box,
    Sphere,
    Count
};

class UElasticBody : public UGameObject
{
    friend class UElasticBodyManager;

public:

    UElasticBody(EElasticBodyShape Shape = EElasticBodyShape::Sphere);
    virtual ~UElasticBody();

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

    //모델 설정
    void SetColor(const Vector4& InColor);

    // 활성화/비활성화
    virtual void Activate() override;
    virtual void DeActivate() override;


private:
    bool bIsActive = true;
    EElasticBodyShape Shape = EElasticBodyShape::Sphere;

    //Rigid Root 빠른 접근
    std::weak_ptr<class URigidBodyComponent> Rigid;
    // 컴포넌트 소유
    std::shared_ptr<class UCollisionComponentBase> Collision;
    std::shared_ptr<class UPrimitiveComponent> Primitive;

};