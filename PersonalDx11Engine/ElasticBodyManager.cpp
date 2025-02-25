#include "ElasticBodyManager.h"

std::shared_ptr<UElasticBody> UElasticBodyManager::SpawnBody(EShape Shape)
{
	auto body = GetBodyFromPool();

	ResetBody(body);

	//상태 설정
	ApplyRandomPhysicsProperties(body);
	ApplyRandomTransform(body);

	//색상 결정
	SetColorBasedOnMass(body);

	//객체 초기화 마무리
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
	// 풀 예약 및 미리 생성
	PrewarmPool(InitialPoolSize);


	bIsInitialized = true;
}

void UElasticBodyManager::Release()
{
	//객체 정리
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

		// 비활성화 상태로 풀에 추가
		newBody->SetActive(false);
		PooledBodies.push_back(newBody);
	}
}


std::shared_ptr<UElasticBody> UElasticBodyManager::CreateNewBody()
{
	auto body = make_shared<UElasticBody>();
	body->PostInitialized(); //최소한의 초기화 및 컴포넌트 구조만 등록
	return body;
}

std::shared_ptr<UElasticBody> UElasticBodyManager::GetBodyFromPool()
{
	return std::shared_ptr<UElasticBody>();
}
