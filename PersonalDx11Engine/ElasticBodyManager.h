#pragma once
#include "ElasticBody.h"
#include <vector>
#include <memory>
#include <unordered_map>

using EShape = UElasticBody::EShape;

class UElasticBodyManager
{
public:
    static UElasticBodyManager* Get()
    {
        static UElasticBodyManager Instance;
        return &Instance;
    }

    // ��ü ���� �� ����
    std::shared_ptr<UElasticBody> SpawnBody(EShape Shape = EShape::Sphere);
    void DespawnBody(const std::shared_ptr<UElasticBody>& Body);
    void DespawnRandomBodies(size_t Count);

    // ���� �Ӽ� ����
    void ApplyRandomPhysicsProperties(const std::shared_ptr<UElasticBody>& Body);
    void ApplyRandomTransform(const std::shared_ptr<UElasticBody>& Body);
    void SetColorBasedOnMass(const std::shared_ptr<UElasticBody>& Body);

    // ���� ī�װ� ����
    enum class EMassCategory
    {
        VeryLight,
        Light,
        Medium,
        Heavy,
        VeryHeavy
    };

    // ���� ���� ���� �޼���
    EMassCategory CategorizeMass(float Mass) const;
    Vector4 GetColorForMassCategory(EMassCategory Category) const;

    // Ǯ�� ����
    void PrewarmPool(size_t Count);
    void UpdateBodies(float DeltaTime);

    // ���� ��ȸ
    size_t GetActiveBodyCount() const { return ActiveBodies.size(); }
    size_t GetPooledBodyCount() const { return PooledBodies.size(); }

private:
    UElasticBodyManager() = default;
    ~UElasticBodyManager() = default;

    // ��ü ����
    std::vector<std::shared_ptr<UElasticBody>> ActiveBodies;
    std::vector<std::shared_ptr<UElasticBody>> PooledBodies;

    // ���� ������
    std::mt19937 RandomGenerator{ std::random_device{}() };

    // �Ӽ� ���� ����
    struct FPropertyRanges
    {
        float MinMass = 0.1f;
        float MaxMass = 10.0f;
        float MinSize = 0.3f;
        float MaxSize = 2.0f;
        float MinRestitution = 0.3f;
        float MaxRestitution = 0.9f;
        float MinFriction = 0.1f;
        float MaxFriction = 0.8f;

        // ��ġ ����
        Vector3 MinPosition{ -20.0f, 0.0f, -20.0f };
        Vector3 MaxPosition{ 20.0f, 10.0f, 20.0f };
    } PropertyRanges;

    // ����-���� ����
    const std::unordered_map<EMassCategory, Vector4> MassColorMap = {
        {EMassCategory::VeryLight, Vector4(0.9f, 0.9f, 1.0f, 1.0f)},
        {EMassCategory::Light, Vector4(0.6f, 0.6f, 1.0f, 1.0f)},
        {EMassCategory::Medium, Vector4(1.0f, 1.0f, 0.6f, 1.0f)},
        {EMassCategory::Heavy, Vector4(1.0f, 0.6f, 0.6f, 1.0f)},
        {EMassCategory::VeryHeavy, Vector4(0.8f, 0.3f, 0.3f, 1.0f)}
    };

    // ��ƿ��Ƽ �޼���
    std::shared_ptr<UElasticBody> CreateNewBody();
    std::shared_ptr<UElasticBody> GetBodyFromPool();
    void ReturnBodyToPool(const std::shared_ptr<UElasticBody>& Body);
};