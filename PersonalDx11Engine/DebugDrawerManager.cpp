#include "DebugDrawerManager.h"
#include "define.h"
#include "ResourceManager.h"
#include "Debug.h"
#include "Renderer.h"
#include "RenderDataSimpleColor.h"
#include "SceneManager.h" 
#include "Model.h"
#include "Material.h"
#include "ConfigReadManager.h"

void UDebugDrawManager::DrawLine(const Vector3& Start, const Vector3& End, const Vector4& Color, float Thickness, float Duration, bool bPersist)
{
	Vector3 Center = (Start + End) * 0.5f;

	float Length = (End - Start).Length();
	Vector3 Extent = Vector3(Length, Thickness, Thickness);

	Vector3 Direciton = (End - Start).GetNormalized();

	//Local X축이 Direciton이 되도록 회전
	Quaternion Rotation = Math::GetRotationBetweenVectors(Vector3(1, 0, 0),Direciton );
	
	DrawBox(Center, Extent, Rotation, Color, Duration, bPersist);
}
void UDebugDrawManager::DrawSphere(const Vector3& Center, float Radius, const Quaternion& Rotation, const Vector4& Color, float Duration, bool bPersist)
{
	auto DrawDebug = FixedPool->AcquireForcely();
	auto DrawDebugPtr = DrawDebug.Get();
	if (!DrawDebugPtr)
	{
		//LOG_FUNC_CALL("Allocation Faied!");
		return;
	}
	DrawDebugPtr->Reset();
	auto Primitive = DrawDebugPtr->Primitive.get();
	if (!Primitive)
	{
		return;
	}

	SetupPrimitive(Primitive, SphereModelHandle_Low,
				   Center, Rotation, 2.0f * Radius * Vector3::One,
				   Color);
	DrawDebugPtr->bPersistent = bPersist;
	DrawDebugPtr->RemainingTime = Duration;

	return;
}
void UDebugDrawManager::DrawBox(const Vector3& Center, const Vector3& Extents, const Quaternion& Rotation, const Vector4& Color, float Duration, bool bPersist)
{
	auto DrawDebug = FixedPool->AcquireForcely();
	auto DrawDebugPtr = DrawDebug.Get();
	if (!DrawDebugPtr)
	{
		//LOG_FUNC_CALL("Allocation Faied!");
		return;
	}
	DrawDebugPtr->Reset();
	auto Primitive = DrawDebugPtr->Primitive.get();
	if (!Primitive)
	{
		return;
	}


	static float accum = 0.0f;
	static const float duration = 1.0f;
	accum += Duration;
	if (accum > duration)
	{
		auto& WolrdTransform = Primitive->GetWorldTransform();
		auto& LocalTransform = Primitive->GetLocalTransform();
		accum = 0.0f;
		LOG("BOX WolrdTransform ==========\n %s \n===============", Debug::ToString(WolrdTransform));
		LOG("BOX LocalTransform ==========\n %s \n===============", Debug::ToString(LocalTransform));
	}

	SetupPrimitive(Primitive, BoxModelHandle,
				   Center, Rotation, Extents,
				   Color);
	DrawDebugPtr->bPersistent = bPersist;
	DrawDebugPtr->RemainingTime = Duration;

	return;
}
void UDebugDrawManager::SetupPrimitive(UPrimitiveComponent* TargetPrimitive,
									   const FResourceHandle& ModelHandle,
									   const Vector3& Position, const Quaternion& Rotation, const Vector3& Scale, 
									   const Vector4& Color)
{
	if (!TargetPrimitive)
		return;

	if(ModelHandle.IsValid())
		TargetPrimitive->SetModel(ModelHandle);

	TargetPrimitive->SetWorldPosition(Position);
	TargetPrimitive->SetWorldRotation(Rotation);
	TargetPrimitive->SetWorldScale(Scale);
	TargetPrimitive->SetColor(Color);

}

UDebugDrawManager::UDebugDrawManager()
{
	FixedPool = std::make_unique< TFixedObjectPool<FDebugShape, DEBUGDRASWER_POOL_SIZE>>();
}

UDebugDrawManager::~UDebugDrawManager()
{
	FixedPool.reset();
}

void UDebugDrawManager::Initialize()
{
	DebugMaterialHandle = UResourceManager::Get()->LoadResource<UMaterial>(MAT_DEFAULT);
	SphereModelHandle_High = UResourceManager::Get()->LoadResource<UModel>(MDL_SPHERE_High);
	SphereModelHandle_Mid = UResourceManager::Get()->LoadResource<UModel>(MDL_SPHERE_Mid);
	SphereModelHandle_Low = UResourceManager::Get()->LoadResource<UModel>(MDL_SPHERE_Low);
	BoxModelHandle = UResourceManager::Get()->LoadResource<UModel>(MDL_CUBE);
}

void UDebugDrawManager::Render(URenderer* InRenderer)
{
	for (const auto& drawObject : *FixedPool)
	{
		auto Draw = drawObject.Get();
		if (!Draw)
		{
			LOG_FUNC_CALL("Invalid Pool Works");
			continue;
		}
		FRenderJob RenderJob = InRenderer->AllocateRenderJob<FRenderDataSimpleColor>();
		RenderJob.RenderState = ERenderStateType::Wireframe;
		auto Primitive = Draw->Primitive.get();
		auto Camera = USceneManager::Get()->GetActiveCamera();
		if (Primitive && Camera)
		{
			if (Primitive->FillRenderData(Camera, RenderJob.RenderData))
			{
				InRenderer->SubmitJob(RenderJob);
			}
		}
	}
}

void UDebugDrawManager::Tick(const float DeltaTime)
{
	using PoolHandle = TFixedObjectPool<UDebugDrawManager::FDebugShape, DEBUGDRASWER_POOL_SIZE>::WeakedObject;
	std::vector<PoolHandle> ToReleased;
	ToReleased.reserve(FixedPool->GetActiveCount());


	for (const auto drawObject : (*FixedPool))
	{
		auto Draw = drawObject.Get();
		if (!Draw)
		{
			LOG_FUNC_CALL("Invalid Pool Works");
			continue;
		}
	
		if (Draw->bPersistent)
			continue;

		Draw->RemainingTime -= DeltaTime;
		if (Draw->RemainingTime < 0.0f)
		{
			Draw->Reset();
			ToReleased.push_back(drawObject);
		}
	}

	//const int PreActiveCount = FixedPool->GetActiveCount();

	for (auto& it : ToReleased)
	{
		it.Release();
	}

	//const int PostActiveCount = FixedPool->GetActiveCount();
	//LOG("return to Pool : [%d]->[%d]", PreActiveCount, PostActiveCount);
}

UDebugDrawManager::FDebugShape::FDebugShape()
{
	Primitive = std::make_unique<UPrimitiveComponent>();

	if (!Primitive)
	{
		LOG_FUNC_CALL("Allocation Failed");
	}
}

void UDebugDrawManager::FDebugShape::Reset()
{
	RemainingTime = 0.0f;
	bPersistent = false;

	if (Primitive)
	{
		//초기화
		Primitive->SetLocalPosition(Vector3::Zero);
		Primitive->SetLocalRotation(Quaternion::Identity);
		Primitive->SetLocalScale(Vector3::One);
		Primitive->SetColor(Vector4(1, 1, 1, 1));
	}
}
