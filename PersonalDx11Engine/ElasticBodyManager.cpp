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

	//���� ����
	ApplyRandomCollisionShape(body);

	//���� ����
	ApplyRandomPhysicsProperties(body);
	ApplyRandomTransform(body);

	//���� ����
	SetColorBasedOnMass(body);

	//��ü �ʱ�ȭ ������
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

void UElasticBodyManager::ClearAllActiveBodies()
{
	// ��� Ȱ�� ��ü�� ��Ȱ��ȭ�ϰ� �ʱ�ȭ
	for (auto& body : ActiveBodies)
	{
		DeactivateBody(body);
	}

	// Ȱ�� ��ü���� �� ���� Ǯ�� �̵�
	if (!ActiveBodies.empty())
	{
		// Ǯ�� ������ ������� Ȯ���ϰ� �ʿ�� Ȯ��
		PooledBodies.reserve(PooledBodies.size() + ActiveBodies.size());

		// std::move�� ����Ͽ� ���� ������ ȿ�������� �̵�
		std::move(std::begin(ActiveBodies), std::end(ActiveBodies),
				  std::back_inserter(PooledBodies));

		 // Ȱ�� ��ü �����̳� ����
		ActiveBodies.clear();
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

	//�ٵ� ��ü Ȱ��ȭ
	body->SetActive(true);

	return body;
}

void UElasticBodyManager::DeactivateBody(std::shared_ptr<UElasticBody>& Body)
{
	if (!Body.get())
	{
		return;
	}
	//��ü ���� �ʱ�ȭ
	Body->Reset();
	//��Ȱ��ȭ
	Body->SetActive(false);
}

void UElasticBodyManager::ReturnBodyToPool(std::shared_ptr<UElasticBody>& Body)
{
	// ��ȿ�� �˻�
	if (!Body.get())
	{
		return;
	}

	// Ȱ�� ��Ͽ��� ���� (�̹� ȣ���ڿ��� ó������ ���� ����)
	auto it = std::find(ActiveBodies.begin(), ActiveBodies.end(), Body);
	if (it != ActiveBodies.end())
	{
		ActiveBodies.erase(it);
	}

	// ��ü ��Ȱ��ȭ
	DeactivateBody(Body);

	// Ǯ�� ��ȯ
	PooledBodies.push_back(Body);
}

