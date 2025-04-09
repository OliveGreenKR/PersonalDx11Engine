#include "DebugDrawerManager.h"
#include "define.h"
#include "ResourceManager.h"
#include "Debug.h"
#include "Renderer.h"
#include "RenderDataSimpleColor.h"
#include "SceneManager.h" 
#include "Model.h"
#include "Material.h"
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
	auto DrawDebug = FixedPool.Acquire();
	auto DrawDebugPtr = DrawDebug.lock();
	if (!DrawDebugPtr)
	{
		LOG_FUNC_CALL("Allocation Faied!");
		return;
	}
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
	auto DrawDebug = FixedPool.Acquire();
	auto DrawDebugPtr = DrawDebug.lock();
	if (!DrawDebugPtr)
	{
		LOG_FUNC_CALL("Allocation Faied!");
		return;
	}
	auto Primitive = DrawDebugPtr->Primitive.get();
	if (!Primitive)
	{
		return;
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

UDebugDrawManager::UDebugDrawManager() : FixedPool()
{
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

	for (const auto& drawObjectWeak : FixedPool)
	{
		auto drawObject = drawObjectWeak.lock();

		if (!drawObject)
		{
			LOG_FUNC_CALL("[Warning] Wrong DebugRender Elements! DebugDrawer no works clearly");
			continue;
		}

		FRenderJob RenderJob = InRenderer->AllocateRenderJob<FRenderDataSimpleColor>();
		RenderJob.RenderState = ERenderStateType::Wireframe;
		auto Primitive = drawObject->Primitive.get();
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
	std::vector<std::weak_ptr<FDebugShape>> ReturnDraws;
	ReturnDraws.reserve(FixedPool.GetActiveCount());

	for (const auto& Draw : FixedPool)
	{
		auto DrawPtr = Draw.lock();
		if (!DrawPtr)
		{
			LOG_FUNC_CALL("[Warning] Pool works inproperly!");
			continue;
		}

		if (DrawPtr->bPersistent)
			continue;

		DrawPtr->RemainingTime -= DeltaTime;
		if (DrawPtr->RemainingTime < 0.0f)
		{
			DrawPtr->Reset();
			ReturnDraws.push_back(Draw);
		}
	}

	for (auto draw : ReturnDraws)
	{
		FixedPool.ReturnToPool(draw);
	}

	//const auto& AfterActiveDraws = Pool.GetActiveObjects();
	//LOG("try return %d to Pool : [%d]->[%d]", ReturnDraws.size(),
	//	ActiveDraws.size(), AfterActiveDraws.size());
}

UDebugDrawManager::FDebugShape::FDebugShape()
{
	Primitive = std::make_unique<UPrimitiveComponent>();

	if (!Primitive)
	{
		LOG_FUNC_CALL("Allocation Failed");
	}
}