#pragma once
#include <vector>
#include <memory>
#include <unordered_map>

class ElasticBody;

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

	// 객체 생성 및 설정
	std::shared_ptr<class UElasticBody> SpawnRandomBody();
	void DespawnBody(std::shared_ptr<UElasticBody>& Body);

	//활성화 개체 수 제한
	void LimitActiveObjectCount(size_t Count);

	//충돌체 속성 관리
	void ApplyRandomCollisionShape(std::shared_ptr<UElasticBody>& Body);

	// 물리 속성 관리
	void ApplyRandomPhysicsProperties(std::shared_ptr<UElasticBody>& Body);
	void ApplyRandomTransform(std::shared_ptr<UElasticBody>& Body);
	void SetColorBasedOnMass(std::shared_ptr<UElasticBody>& Body);

	//최소 Count만큼의 Pool 미리 준비
	void PrewarmPool(size_t Count); 

	void UpdateActiveBodies(float DeltaTime);
	void ClearAllActiveBodies();

	// 상태 조회
	size_t GetActiveBodyCount() const { return ActiveBodies.size(); }
	size_t GetPooledBodyCount() const { return PooledBodies.size(); }

private:
	UElasticBodyManager();
	~UElasticBodyManager();

	void Initialize(size_t InitialPoolSize = 128);
	void Release();

	bool bIsInitialized = false;

	//객체 비활성화
	void DeactivateBody(std::shared_ptr<UElasticBody>& Body);

	// 질량 카테고리 구분
	enum EMassCategory
	{
		VeryLight = 0,
		Light,
		Medium,
		Heavy,
		VeryHeavy,
		Count
	};

	// 질량 범주 결정 메서드
	EMassCategory CategorizeMass(const float Mass) const;
	Vector4 GetColorForMassCategory(const EMassCategory Category) const;

	// 객체 관리 Pool
	std::vector<std::shared_ptr<UElasticBody>> ActiveBodies;
	std::vector<std::shared_ptr<UElasticBody>> PooledBodies;

	// 속성 범위 설정
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
		float MaxSpeed = 100.0f; 
		float MaxAngularSpeed = 6.0f * PI;

		// 위치 범위
		Vector3 MinPosition{ -3.0f, -3.0f, -5.0f };
		Vector3 MaxPosition{ 3.0f, 3.0f, 5.0f };
	} PropertyRanges;

	// 질량-색상 매핑
	const std::unordered_map<EMassCategory, Vector4> MassColorMap = {
		{EMassCategory::VeryLight,  Vector4(1.0f, 1.0f, 1.0f, 1.0f)},  // White
		{EMassCategory::Light,      Vector4(1.0f, 1.0f, 0.0f, 1.0f)},  // Yellow
		{EMassCategory::Medium,     Vector4(0.0f, 1.0f, 0.0f, 1.0f)},  // Green
		{EMassCategory::Heavy,      Vector4(0.0f, 0.0f, 0.5f, 1.0f)},  // Dark Blue
		{EMassCategory::VeryHeavy,  Vector4(0.3f, 0.3f, 0.3f, 1.0f)}   // Dark Gray
	};

	// 유틸리티 메서드
	std::shared_ptr<UElasticBody> CreateNewBody();
	std::shared_ptr<UElasticBody> GetBodyFromPool();
	void ReturnBodyToPool(std::shared_ptr<UElasticBody>& Body);


};