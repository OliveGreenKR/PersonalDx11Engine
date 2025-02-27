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

    // �⺻ ���� �ֱ� �޼���
    virtual void Tick(const float DeltaTime) override;
    virtual void PostInitialized() override;
    virtual void PostInitializedComponents() override;

    // ������Ʈ ������
    class URigidBodyComponent* GetRigidBody() const { return Rigid.lock().get(); }
    class UCollisionComponent* GetCollisionComponent() const { return Collision.lock().get(); }

    /// <summary>
    /// �ν��Ͻ� ���� �ʱ�ȭ
    /// </summary>
    virtual void Reset();

    // �浹 �̺�Ʈ �ڵ鷯
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

    // ���� �Ӽ� ����  
    void SetMass(float InMass);
    void SetMaxSpeed(float InSpeed);
    void SetMaxAngularSpeed(float InSpeed);
    void SetGravityScale(float InScale);
    void SetFrictionKinetic(float InFriction);
    void SetFrictionStatic(float InFriction);
    void SetRestitution(float InRestitution);

    //�浹ü ����
    void SetShape(EShape InShape);
    void SetShapeSphere();
    void SetShapeBox();

    // Ȱ��ȭ/��Ȱ��ȭ
    void SetActive(bool bActive);
    bool IsActive() const { return bIsActive; }

private:
    // ���� ��ƿ��Ƽ �޼���
    enum class ECollisionShapeType GetCollisionShape(const EShape Shape) const;

private:
    bool bIsActive = true;
    EShape Shape = EShape::Sphere;


    // ������Ʈ ����
    std::weak_ptr<class URigidBodyComponent> Rigid;
    std::weak_ptr<class UCollisionComponent> Collision;

};