#pragma once
#include <vector>
#include <memory>
#include <unordered_map>
#include "Math.h" 

class UElasticBodyManager
{
public:
	UElasticBodyManager();
	~UElasticBodyManager();

	// 복사/이동 및 대입 연산자 삭제
	UElasticBodyManager(const UElasticBodyManager&) = delete;
	UElasticBodyManager& operator=(const UElasticBodyManager&) = delete;
	UElasticBodyManager(UElasticBodyManager&&) = delete;
	UElasticBodyManager& operator=(UElasticBodyManager&&) = delete;


public:
	static UElasticBodyManager* Get()
	{
		// 포인터를 static으로 선언
		static UElasticBodyManager* instance = []() {
			UElasticBodyManager* manager = new UElasticBodyManager();
			manager->Initialize();
			return manager;
			}();

		return instance;
	}

	// 객체 생성 및 설정
	std::shared_ptr<class UElasticBody> SpawnRandomBody();
	void DespawnRandomBody();

	//활성화 개체 수 제한
	void LimitActiveObjectCount(const size_t Count);

	//충돌체 속성 관리
	void ApplyRandomShape(std::shared_ptr<UElasticBody>& Body);

	// 물리 속성 관리
	void ApplyRandomPhysicsProperties(std::shared_ptr<UElasticBody>& Body);
	void ApplyRandomTransform(std::shared_ptr<UElasticBody>& Body);
	void SetColorBasedOnMass(std::shared_ptr<UElasticBody>& Body);

	//최소 Count만큼의 Pool 미리 준비
	void PrewarmPool(size_t Count); 

	void Tick(float DeltaTime);
	void Render(class URenderer* InRenderer, 
				class UCamera* InCamera, 
				class UShader* InShader, 
				class ID3D11ShaderResourceView* InTexture, 
				class ID3D11SamplerState* InCustomSampler = nullptr);

	void ClearAllActiveBodies();

	// 상태 조회
	size_t GetActiveBodyCount() const { return ActiveBodies.size(); }
	size_t GetPooledBodyCount() const { return PooledBodies.size(); }
private:
	//초기화 
	void Initialize(size_t InitialPoolSize = 128);
	//자원 회수
	void Release();

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
	Vector4 GetColorFromMassCategory(const EMassCategory Category) const;

	// 객체 관리 Pool
	std::vector<std::shared_ptr<UElasticBody>> ActiveBodies;
	std::vector<std::shared_ptr<UElasticBody>> PooledBodies;

	// 속성 범위 설정
	struct FPropertyRanges
	{
		float MinMass = 0.5f;
		float MaxMass = 10.0f;
		float MinSize = 0.3f;
		float MaxSize = 1.0f;
		float MinRestitution = 0.3f;
		float MaxRestitution = 0.9f;
		float MinFriction = 0.1f;
		float MaxFriction = 0.6f;
		float MaxSpeed = 100.0f; 
		float MaxAngularSpeed = 6.0f * PI;

		// 위치 범위
		Vector3 MinPosition{ -2.0f, -2.0f, 0.0f };
		Vector3 MaxPosition{ 2.0f, 2.0f, 0.0f };
	} PropertyRanges;

	// 질량-색상 매핑
	std::unordered_map<EMassCategory, Vector4> MassColorMap;

	// 유틸리티 메서드
	std::shared_ptr<UElasticBody> CreateNewBody();
	std::shared_ptr<UElasticBody> GetBodyFromPool();
	void ReturnBodyToPool(std::shared_ptr<UElasticBody>& Body);
};