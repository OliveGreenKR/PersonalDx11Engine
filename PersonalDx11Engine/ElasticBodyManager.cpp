#include "ElasticBodyManager.h"
#include "Math.h"
#include "Random.h"
#include "ElasticBody.h"


UElasticBodyManager::UElasticBodyManager()
{
	Initialize();
}

UElasticBodyManager::~UElasticBodyManager()
{
	Release();

}

std::shared_ptr<UElasticBody> UElasticBodyManager::SpawnRandomBody()
{
	auto body = GetBodyFromPool();

	if (!body.get())
		return nullptr;

	//형태 결정
	ApplyRandomCollisionShape(body);

	//상태 설정
	ApplyRandomPhysicsProperties(body);
	ApplyRandomTransform(body);

	//색상 결정
	SetColorBasedOnMass(body);

	//객체 초기화 마무리
	body.get()->SetActive(true);
	body.get()->PostInitializedComponents();

	return body;
}

void UElasticBodyManager::DespawnBody(std::shared_ptr<UElasticBody>& Body)
{
	ReturnBodyToPool(Body);
}

void UElasticBodyManager::ApplyRandomCollisionShape(std::shared_ptr<UElasticBody>& Body)
{
	UElasticBody::EShape randShape = (UElasticBody::EShape)FRandom::RandI(0, (int)UElasticBody::EShape::Count);
	Body->SetShape(randShape);
}

void UElasticBodyManager::ApplyRandomPhysicsProperties(std::shared_ptr<UElasticBody>& Body)
{
	if (!Body.get())
		return;
	Body->SetMaxSpeed(PropertyRanges.MaxSpeed);
	Body->SetMaxAngularSpeed(PropertyRanges.MaxAngularSpeed);
	Body->SetMass(FRandom::RandF(PropertyRanges.MinMass, PropertyRanges.MaxMass));
	Body->SetRestitution(FRandom::RandF(PropertyRanges.MinRestitution, PropertyRanges.MaxRestitution));
	Body->SetFrictionKinetic(FRandom::RandF(PropertyRanges.MinFriction, PropertyRanges.MaxFriction));
	Body->SetFrictionStatic(FRandom::RandF(PropertyRanges.MinFriction, PropertyRanges.MaxFriction));
}

void UElasticBodyManager::ApplyRandomTransform(std::shared_ptr<UElasticBody>& Body)
{
	if (!Body.get())
		return;

	Vector3 RandPos = FRandom::RandVector(PropertyRanges.MinPosition, PropertyRanges.MaxPosition);
	Vector3 RandScale = Vector3::One * FRandom::RandF(PropertyRanges.MinSize, PropertyRanges.MaxSize);
	Quaternion RandQuat = FRandom::RandQuat();
	Body->GetTransform()->SetPosition(RandPos);
	Body->GetTransform()->SetScale(RandScale);
	Body->GetTransform()->SetRotation(RandQuat);
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

void UElasticBodyManager::ClearAllActiveBodies()
{
	// 모든 활성 객체를 비활성화하고 초기화
	for (auto& body : ActiveBodies)
	{
		DeactivateBody(body);
	}

	// 활성 객체들을 한 번에 풀로 이동
	if (!ActiveBodies.empty())
	{
		// 풀에 공간이 충분한지 확인하고 필요시 확장
		PooledBodies.reserve(PooledBodies.size() + ActiveBodies.size());

		// std::move를 사용하여 벡터 내용을 효율적으로 이동
		std::move(std::begin(ActiveBodies), std::end(ActiveBodies),
				  std::back_inserter(PooledBodies));

		 // 활성 객체 컨테이너 비우기
		ActiveBodies.clear();
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

	//바디 객체 활성화
	body->SetActive(true);

	return body;
}

void UElasticBodyManager::DeactivateBody(std::shared_ptr<UElasticBody>& Body)
{
	if (!Body.get())
	{
		return;
	}
	//객체 상태 초기화
	Body->Reset();
	//비활성화
	Body->SetActive(false);
}

void UElasticBodyManager::ReturnBodyToPool(std::shared_ptr<UElasticBody>& Body)
{
	// 유효성 검사
	if (!Body.get())
	{
		return;
	}

	// 활성 목록에서 제거 (이미 호출자에서 처리했을 수도 있음)
	auto it = std::find(ActiveBodies.begin(), ActiveBodies.end(), Body);
	if (it != ActiveBodies.end())
	{
		ActiveBodies.erase(it);
	}

	// 객체 비활성화
	DeactivateBody(Body);

	// 풀에 반환
	PooledBodies.push_back(Body);
}

