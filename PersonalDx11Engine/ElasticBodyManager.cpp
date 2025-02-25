#include "ElasticBodyManager.h"
#include "Math.h"

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
	// 풀이 비어있으면 추가 객체 생성
	if (PooledBodies.empty())
	{

		const size_t BatchSize = ActiveBodies.size() * 0.5f;
		PrewarmPool(BatchSize);

		// 그래도 비어있다면 오류 상황 (메모리 부족 등)
		if (PooledBodies.empty())
		{
			// 로그 출력 또는 예외 처리
			return nullptr;
		}
	}

	// 풀의 마지막 객체를 가져옴 (LIFO - 스택 방식으로 최근에 반환된 객체 우선 사용)
	std::shared_ptr<UElasticBody> body = PooledBodies.back();
	PooledBodies.pop_back();

	return body;
}
