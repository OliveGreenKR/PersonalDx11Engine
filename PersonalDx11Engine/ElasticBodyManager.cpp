#include "ElasticBodyManager.h"
#include "Math.h"

std::shared_ptr<UElasticBody> UElasticBodyManager::SpawnBody(EShape Shape)
{
	auto body = GetBodyFromPool();

	ResetBody(body);

	//���� ����
	ApplyRandomPhysicsProperties(body);
	ApplyRandomTransform(body);

	//���� ����
	SetColorBasedOnMass(body);

	//��ü �ʱ�ȭ ������
	body->SetActive(true);
	body->PostInitializedComponents();

	return body;
}

void UElasticBodyManager::ApplyRandomPhysicsProperties(const std::shared_ptr<UElasticBody>& Body)
{


}


UElasticBodyManager::UElasticBodyManager()
{
	Initialize();
}

UElasticBodyManager::~UElasticBodyManager()
{
	Release();
}

void UElasticBodyManager::Initialize(size_t InitialPoolSize)
{
	// Ǯ ���� �� �̸� ����
	PrewarmPool(InitialPoolSize);


	bIsInitialized = true;
}

void UElasticBodyManager::Release()
{
	//��ü ����
	ClearAllActiveBodies();
	PooledBodies.clear();
	bIsInitialized = false;
}

UElasticBodyManager::EMassCategory UElasticBodyManager::CategorizeMass(const float Mass) const
{
	if (Mass < PropertyRanges.MinMass)
		return (EMassCategory)0;

	if (Mass > PropertyRanges.MaxMass)
		return (EMassCategory)(EMassCategory::Count - 1);

	float StepMass = (PropertyRanges.MaxMass - PropertyRanges.MinMass) / (EMassCategory::Count);

	size_t Category = static_cast<size_t>((Mass - PropertyRanges.MinMass) / StepMass);

	return static_cast<EMassCategory>(Category);
}

Vector4 UElasticBodyManager::GetColorForMassCategory(const EMassCategory Category) const
{
	auto it = MassColorMap.find(Category);
	if (it != MassColorMap.end())
	{
		return it->second;
	}
	
	return Vector4(0, 0, 0, 1);
}

void UElasticBodyManager::PrewarmPool(size_t Count)
{
	if (PooledBodies.size() >= Count) 
		return;

	size_t toCreate = Count - PooledBodies.size();
	for (size_t i = 0; i < toCreate; ++i)
	{
		auto newBody = CreateNewBody();

		// ��Ȱ��ȭ ���·� Ǯ�� �߰�
		newBody->SetActive(false);
		PooledBodies.push_back(newBody);
	}
}


std::shared_ptr<UElasticBody> UElasticBodyManager::CreateNewBody()
{
	auto body = make_shared<UElasticBody>();
	body->PostInitialized(); //�ּ����� �ʱ�ȭ �� ������Ʈ ������ ���
	return body;
}

std::shared_ptr<UElasticBody> UElasticBodyManager::GetBodyFromPool()
{
	// Ǯ�� ��������� �߰� ��ü ����
	if (PooledBodies.empty())
	{

		const size_t BatchSize = ActiveBodies.size() * 0.5f;
		PrewarmPool(BatchSize);

		// �׷��� ����ִٸ� ���� ��Ȳ (�޸� ���� ��)
		if (PooledBodies.empty())
		{
			// �α� ��� �Ǵ� ���� ó��
			return nullptr;
		}
	}

	// Ǯ�� ������ ��ü�� ������ (LIFO - ���� ������� �ֱٿ� ��ȯ�� ��ü �켱 ���)
	std::shared_ptr<UElasticBody> body = PooledBodies.back();
	PooledBodies.pop_back();

	return body;
}
