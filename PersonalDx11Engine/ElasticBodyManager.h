#pragma once
#include <vector>
#include <memory>
#include <unordered_map>
#include "Math.h" 

class ElasticBody;

class UElasticBodyManager
{
private:
	UElasticBodyManager();
	~UElasticBodyManager();

	// ����/�̵� �� ���� ������ ����
	UElasticBodyManager(const UElasticBodyManager&) = delete;
	UElasticBodyManager& operator=(const UElasticBodyManager&) = delete;
	UElasticBodyManager(UElasticBodyManager&&) = delete;
	UElasticBodyManager& operator=(UElasticBodyManager&&) = delete;


public:
	static UElasticBodyManager* Get()
	{
		// �����͸� static���� ����
		static UElasticBodyManager* instance = []() {
			UElasticBodyManager* manager = new UElasticBodyManager();
			manager->Initialize();
			return manager;
			}();

		return instance;
	}

	// ��ü ���� �� ����
	std::shared_ptr<class UElasticBody> SpawnRandomBody();
	void DespawnRandomBody();

	//Ȱ��ȭ ��ü �� ����
	void LimitActiveObjectCount(const size_t Count);

	//�浹ü �Ӽ� ����
	void ApplyRandomShape(std::shared_ptr<UElasticBody>& Body);

	// ���� �Ӽ� ����
	void ApplyRandomPhysicsProperties(std::shared_ptr<UElasticBody>& Body);
	void ApplyRandomTransform(std::shared_ptr<UElasticBody>& Body);
	void SetColorBasedOnMass(std::shared_ptr<UElasticBody>& Body);

	//�ּ� Count��ŭ�� Pool �̸� �غ�
	void PrewarmPool(size_t Count); 

	void Tick(float DeltaTime);
	void Render(class URenderer* InRenderer, 
				class UCamera* InCamera, 
				class UShader* InShader, 
				class ID3D11ShaderResourceView* InTexture, 
				class ID3D11SamplerState* InCustomSampler = nullptr);

	void ClearAllActiveBodies();

	// ���� ��ȸ
	size_t GetActiveBodyCount() const { return ActiveBodies.size(); }
	size_t GetPooledBodyCount() const { return PooledBodies.size(); }
private:
	//�ʱ�ȭ 
	void Initialize(size_t InitialPoolSize = 128);
	//�ڿ� ȸ��
	void Release();

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
	Vector4 GetColorFromMassCategory(const EMassCategory Category) const;

	// ��ü ���� Pool
	std::vector<std::shared_ptr<UElasticBody>> ActiveBodies;
	std::vector<std::shared_ptr<UElasticBody>> PooledBodies;

	// �Ӽ� ���� ����
	struct FPropertyRanges
	{
		float MinMass = 0.1f;
		float MaxMass = 10.0f;
		float MinSize = 0.3f;
		float MaxSize = 1.0f;
		float MinRestitution = 0.3f;
		float MaxRestitution = 0.9f;
		float MinFriction = 0.1f;
		float MaxFriction = 0.8f;
		float MaxSpeed = 100.0f; 
		float MaxAngularSpeed = 6.0f * PI;

		// ��ġ ����
		Vector3 MinPosition{ -3.0f, -3.0f, -5.0f };
		Vector3 MaxPosition{ 3.0f, 3.0f, 5.0f };
	} PropertyRanges;

	// ����-���� ����
	std::unordered_map<EMassCategory, Vector4> MassColorMap;

	// ���� - �� ����
	std::unordered_map<int, std::shared_ptr<class UModel>> ModelMap;

	// ��ƿ��Ƽ �޼���
	std::shared_ptr<UElasticBody> CreateNewBody();
	std::shared_ptr<UElasticBody> GetBodyFromPool();
	void ReturnBodyToPool(std::shared_ptr<UElasticBody>& Body);


};