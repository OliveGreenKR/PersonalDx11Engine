#include "ElasticBodyManager.h"
#include "Math.h"
#include "Random.h"
#include "ElasticBody.h"
#include "Renderer.h"
#include "Model.h"
#include "ModelBufferManager.h"


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
	ApplyRandomShape(body);

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

void UElasticBodyManager::DespawnRandomBody()
{
	const size_t count = ActiveBodies.size();
	if (count < 1)
		return;
	size_t dltIndex = FRandom::RandI(0, count - 1);
	ReturnBodyToPool(ActiveBodies[dltIndex]);
}

void UElasticBodyManager::ApplyRandomShape(std::shared_ptr<UElasticBody>& Body)
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
	using EShape = UElasticBody::EShape;
	//��� �ʱ�ȭ
	MassColorMap = {
		{EMassCategory::VeryLight,  Vector4(1.0f, 1.0f, 1.0f, 1.0f)},  // White
		{EMassCategory::Light,      Vector4(1.0f, 1.0f, 0.0f, 1.0f)},  // Yellow
		{EMassCategory::Medium,     Vector4(0.0f, 1.0f, 0.0f, 1.0f)},  // Green
		{EMassCategory::Heavy,      Vector4(0.0f, 0.0f, 0.5f, 1.0f)},  // Dark Blue
		{EMassCategory::VeryHeavy,  Vector4(0.3f, 0.3f, 0.3f, 1.0f)}   // Dark Gray
	};

	// Ǯ ���� �� �̸� ����
	PrewarmPool(InitialPoolSize);
}

void UElasticBodyManager::Release()
{
	//��ü ����
	ClearAllActiveBodies();
	PooledBodies.clear();
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

Vector4 UElasticBodyManager::GetColorFromMassCategory(const EMassCategory Category) const
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

	ActiveBodies.push_back(body);
	//�ٵ� ��ü Ȱ��ȭ
	body->SetActive(true);
	//�ٵ� �ʱ�ȭ ������
	body->PostInitializedComponents();

	return body;
}

void UElasticBodyManager::DeactivateBody(std::shared_ptr<UElasticBody>& Body)
{
	if (!Body.get())
	{
		return;
	}
	//��Ȱ��ȭ
	Body->SetActive(false);
	//��ü ���� �ʱ�ȭ
	Body->Reset();
	
}

void UElasticBodyManager::ReturnBodyToPool(std::shared_ptr<UElasticBody>& Body)
{
	// ��ȿ�� �˻�
	if (!Body.get())
	{
		return;
	}

	// Ȱ�� ��Ͽ��� ���� 
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

void UElasticBodyManager::SetColorBasedOnMass(std::shared_ptr<UElasticBody>& Body)
{
	if (!Body.get())
		return;
	EMassCategory Category = CategorizeMass(Body->GetMass());
	Body->SetDebugColor(GetColorFromMassCategory(Category));
}

void UElasticBodyManager::Tick(float DeltaTime)
{
	for (auto body : ActiveBodies)
	{
		if (!body.get())
		{
			continue;
		}
		body->Tick(DeltaTime);
	}
}

void UElasticBodyManager::Render(URenderer* InRenderer, UCamera* InCamera, UShader* InShader, ID3D11ShaderResourceView* InTexture, ID3D11SamplerState* InCustomSampler)
{
	if (!InRenderer || !InCamera || !InTexture)
		return;
	
	for (auto body : ActiveBodies)
	{
		if (!body.get())
			continue;
		if (body->bIsActive)
		{
			InRenderer->RenderGameObject(InCamera, body.get(), InShader, InCustomSampler);
		}
	}
}


//to imple
void UElasticBodyManager::LimitActiveObjectCount(const size_t Count)
{
	//Count ������ Ȱ��ȭ ��ü ���� ����(�߰� �� ����)
	const size_t targetCount = Math::Max(0, Count);

}



