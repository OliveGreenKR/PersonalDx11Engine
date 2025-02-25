#include "ElasticBodyManager.h"

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
	return std::shared_ptr<UElasticBody>();
}
