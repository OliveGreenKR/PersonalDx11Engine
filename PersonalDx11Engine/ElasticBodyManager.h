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

    // 객체 생성 및 설정
    std::shared_ptr<UElasticBody> SpawnBody(EShape Shape = EShape::Sphere);
    void DespawnBody(const std::shared_ptr<UElasticBody>& Body);
    void DespawnRandomBodies(size_t Count);

    // 물리 속성 관리
    void ApplyRandomPhysicsProperties(const std::shared_ptr<UElasticBody>& Body);
    void ApplyRandomTransform(const std::shared_ptr<UElasticBody>& Body);
    void SetColorBasedOnMass(const std::shared_ptr<UElasticBody>& Body);

    // 질량 카테고리 구분
    enum class EMassCategory
    {
        VeryLight,
        Light,
        Medium,
        Heavy,
        VeryHeavy
    };

    // 질량 범주 결정 메서드
    EMassCategory CategorizeMass(float Mass) const;
    Vector4 GetColorForMassCategory(EMassCategory Category) const;

    // 풀링 관리
    void PrewarmPool(size_t Count);
    void UpdateBodies(float DeltaTime);

    // 상태 조회
    size_t GetActiveBodyCount() const { return ActiveBodies.size(); }
    size_t GetPooledBodyCount() const { return PooledBodies.size(); }

private:
    UElasticBodyManager() = default;
    ~UElasticBodyManager() = default;

    // 객체 관리
    std::vector<std::shared_ptr<UElasticBody>> ActiveBodies;
    std::vector<std::shared_ptr<UElasticBody>> PooledBodies;

    // 랜덤 생성기
    std::mt19937 RandomGenerator{ std::random_device{}() };

    // 속성 범위 설정
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

        // 위치 범위
        Vector3 MinPosition{ -20.0f, 0.0f, -20.0f };
        Vector3 MaxPosition{ 20.0f, 10.0f, 20.0f };
    } PropertyRanges;

    // 질량-색상 매핑
    const std::unordered_map<EMassCategory, Vector4> MassColorMap = {
        {EMassCategory::VeryLight, Vector4(0.9f, 0.9f, 1.0f, 1.0f)},
        {EMassCategory::Light, Vector4(0.6f, 0.6f, 1.0f, 1.0f)},
        {EMassCategory::Medium, Vector4(1.0f, 1.0f, 0.6f, 1.0f)},
        {EMassCategory::Heavy, Vector4(1.0f, 0.6f, 0.6f, 1.0f)},
        {EMassCategory::VeryHeavy, Vector4(0.8f, 0.3f, 0.3f, 1.0f)}
    };

    // 유틸리티 메서드
    std::shared_ptr<UElasticBody> CreateNewBody();
    std::shared_ptr<UElasticBody> GetBodyFromPool();
    void ReturnBodyToPool(const std::shared_ptr<UElasticBody>& Body);
};