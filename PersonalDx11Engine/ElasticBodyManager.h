#pragma once
#include "ElasticBody.h"
#include <vector>
#include <memory>
#include <unordered_map>
#include "Random.h"

using EShape = UElasticBody::EShape;

class UElasticBodyManager
{
public:
	static UElasticBodyManager* Get()
	{
		static UElasticBodyManager Instance;
		if (!Instance.bIsInitialized)
		{
			Instance.Initialize();
		}
		return &Instance;
	}

	// ��ü ���� �� ����

	std::shared_ptr<UElasticBody> SpawnBody(const EShape Shape = EShape::Sphere);
	void DespawnBody(std::shared_ptr<UElasticBody>& Body);
	void DespawnRandomBodies(size_t Count);

	// ���� �Ӽ� ����

	void ApplyRandomPhysicsProperties(std::shared_ptr<UElasticBody>& Body);
	void ApplyRandomTransform(std::shared_ptr<UElasticBody>& Body);
	void SetColorBasedOnMass(std::shared_ptr<UElasticBody>& Body);

	//�ּ� Count��ŭ�� Pool �̸� �غ�
	void PrewarmPool(size_t Count); 

	void UpdateActiveBodies(float DeltaTime);
	void ClearAllActiveBodies();

	// ���� ��ȸ
	size_t GetActiveBodyCount() const { return ActiveBodies.size(); }
	size_t GetPooledBodyCount() const { return PooledBodies.size(); }

private:
	UElasticBodyManager();
	~UElasticBodyManager();

	void Initialize(size_t InitialPoolSize = 128);
	void Release();

	bool bIsInitialized = false;

	//��ü ��Ȱ��ȭ
	void DeactivateBody(std::shared_ptr<UElasticBody>& Body);

	// ���� ī�װ� ����
	enum EMassCategory
	{
		VeryLight = 0,
		Light,
		Medium,
		Heavy,
		VeryHeavy,
		Count
	};

	// ���� ���� ���� �޼���
	EMassCategory CategorizeMass(const float Mass) const;
	Vector4 GetColorForMassCategory(const EMassCategory Category) const;

	// ��ü ���� Pool
	std::vector<std::shared_ptr<UElasticBody>> ActiveBodies;
	std::vector<std::shared_ptr<UElasticBody>> PooledBodies;

	// �Ӽ� ���� ����
	struct FPropertyRanges
	{
		float MinMass = 0.1f;
		float MaxMass = 10.0f;
		float MinSize = 0.1f;
		float MaxSize = 1.0f;
		float MinRestitution = 0.3f;
		float MaxRestitution = 0.9f;
		float MinFriction = 0.1f;
		float MaxFriction = 0.8f;

		// ��ġ ����
		Vector3 MinPosition{ -3.0f, -3.0f, -5.0f };
		Vector3 MaxPosition{ 3.0f, 3.0f, 5.0f };
	} PropertyRanges;

	// ����-���� ����
	const std::unordered_map<EMassCategory, Vector4> MassColorMap = {
		{EMassCategory::VeryLight,  Vector4(1.0f, 1.0f, 1.0f, 1.0f)},  // White
		{EMassCategory::Light,      Vector4(1.0f, 1.0f, 0.0f, 1.0f)},  // Yellow
		{EMassCategory::Medium,     Vector4(0.0f, 1.0f, 0.0f, 1.0f)},  // Green
		{EMassCategory::Heavy,      Vector4(0.0f, 0.0f, 0.5f, 1.0f)},  // Dark Blue
		{EMassCategory::VeryHeavy,  Vector4(0.3f, 0.3f, 0.3f, 1.0f)}   // Dark Gray
	};

	// ��ƿ��Ƽ �޼���
	std::shared_ptr<UElasticBody> CreateNewBody();
	std::shared_ptr<UElasticBody> GetBodyFromPool();
	void ReturnBodyToPool(std::shared_ptr<UElasticBody>& Body);
};