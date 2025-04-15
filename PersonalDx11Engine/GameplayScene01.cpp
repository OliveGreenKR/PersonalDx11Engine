#pragma once
#include "GameplayScene01.h"
#include "Renderer.h"
#include "RigidBodyComponent.h"
#include "CollisionComponent.h"
#include "PrimitiveComponent.h"

#include "Material.h"
#include "CollisionManager.h"
#include "InputManager.h"
#include "Random.h"
#include "define.h"

#include "Debug.h"
#include "ResourceManager.h"
#include "Texture.h"
#include "UIManager.h" 

#include "RenderDataTexture.h"

UGameplayScene01::UGameplayScene01()
{
    InputContext = UInputContext::Create(SceneName);
}

UGameplayScene01::~UGameplayScene01()
{
    if (ElasticBodyPool)
    {
        ElasticBodyPool->ClearAllActives();
        ElasticBodyPool.reset();
    }
    if (InputContext)
    {
        InputContext = nullptr;
    }
}

void UGameplayScene01::Initialize()  
{ 
    const int VIEW_WIDTH = 800;
    const int VIEW_HEIGHT = 800;

    // 카메라 설정
    Camera = UCamera::Create(PI / 4.0f, VIEW_WIDTH, VIEW_HEIGHT, 0.1f, 100.0f);
    Camera->SetPosition({ 0, 0.0f, -10.0f });

    // 입력 설정
    SetupInput();

    bSpawnBody = false;

}

void UGameplayScene01::Load()
{
    // 입력 컨텍스트 등록
    UInputManager::Get()->RegisterInputContext(InputContext);

    //매터리얼 로드
    //TileMaterialHandle = UResourceManager::Get()->LoadResource<UMaterial>(MAT_TILE);
    PoleMaterialHandle = UResourceManager::Get()->LoadResource<UMaterial>(MAT_POLE);
    //DefaultMaterialHandle = UResourceManager::Get()->LoadResource<UMaterial>(MAT_DEFAULT);

    ElasticBodyPool = std::make_unique< TFixedObjectPool<UElasticBody, 512>>();
}

void UGameplayScene01::Unload()
{
    // 입력 컨텍스트 삭제
    UInputManager::Get()->UnregisterInputContext(SceneName);

    //활성화 바디 상태 초기화
    //객체풀 클리어
    if (ElasticBodyPool)
    {
        ElasticBodyPool.reset();
    }
   
    // 주요 객체 해제
    Camera = nullptr;
}

void UGameplayScene01::Tick(float DeltaTime)
{
    if (Camera)
    {
        Camera->Tick(DeltaTime);
    }

    // 탄성체 스폰 로직
    AccumTime += DeltaTime;
    if (AccumTime > SPAWN_FREQUENCY)
    {
        AccumTime = 0.0f;
        if (bSpawnBody)
        {
            SpawnElasticBody();
        }
    }

    // 모든 탄성체 업데이트
    for (auto body : *ElasticBodyPool)
    {
        body.Get()->Tick(DeltaTime);
    }

    BodyNum = ElasticBodyPool->GetActiveCount();
}

void UGameplayScene01::SubmitRender(URenderer* Renderer)
{
    for (auto body : *ElasticBodyPool)
    {
        FRenderJob RenderJob = Renderer->AllocateRenderJob<FRenderDataTexture>();
        auto Primitive = body.Get()->GetComponentByType<UPrimitiveComponent>();
        if (Primitive)
        {
            if (Primitive->FillRenderData(GetMainCamera(), RenderJob.RenderData))
            {
                Renderer->SubmitJob(RenderJob);
            }
        }
    }
}

void UGameplayScene01::SubmitRenderUI()
{
    const ImGuiWindowFlags UIWindowFlags =
        ImGuiWindowFlags_NoResize |      // 크기 조절 비활성화
        ImGuiWindowFlags_AlwaysAutoResize;  // 항상 내용에 맞게 크기 조절

    UUIManager::Get()->RegisterUIElement("Scene01UI", [this]() {

        if (Camera)
        {
            ImGui::Begin("Camera", nullptr, UIWindowFlags);
            ImGui::Checkbox("bIs2D", &Camera->bIs2D);
            ImGui::Checkbox("bLookAtObject", &Camera->bLookAtObject);
            ImGui::Text(Debug::ToString(Camera->GetTransform()));
            ImGui::End();
        }

        ImGui::Begin("ElasticBodies", nullptr, UIWindowFlags);
        ImGui::Checkbox("bSpawnBody", &bSpawnBody);
        if (ImGui::Checkbox("bGravity", &bGravity))
        {
            for (auto body : *ElasticBodyPool)
            {
                body.Get()->SetGravity(bGravity);
            }
        }
        if(ImGui::Button("Spawn"))
        {
            SpawnElasticBody();
        }
        ImGui::SameLine();
        if (ImGui::Button("DeSpawn"))
        {
            DeSpawnElasticBody();
        }
        if (ImGui::InputInt("BodyCount", &BodyNum, 1, 10))
        {
            //Clamp target Num
            BodyNum = Math::Clamp(BodyNum, 0, 512);
            int delta = BodyNum - ElasticBodyPool->GetActiveCount();
            int count = std::abs(delta);
            for (int i = 0; i < count; ++i)
            {
                if (delta > 0)
                {
                    SpawnElasticBody();
                }
                else
                {
                    DeSpawnElasticBody();
                }
            }

        }
        ImGui::End();
                                           });
}

void UGameplayScene01::HandleInput(const FKeyEventData& EventData)
{
    // 이 함수는 사용되지 않을 수 있음 - InputContext가 대부분의 입력 처리를 담당
    // 필요한 경우 직접적인 입력 처리를 여기에 추가
}

void UGameplayScene01::SetupInput()
{
    UInputAction CameraUp("CameraUp");
    CameraUp.KeyCodes = { VK_UP };

    UInputAction CameraDown("CameraDown");
    CameraDown.KeyCodes = { VK_DOWN };

    UInputAction CameraRight("CameraRight");
    CameraRight.KeyCodes = { VK_RIGHT };

    UInputAction CameraLeft("CameraLeft");
    CameraLeft.KeyCodes = { VK_LEFT };

    UInputAction CameraFollowObject("CameraFollowObject");
    CameraFollowObject.KeyCodes = { 'V' };

    UInputAction CameraLookTo("CameraLookTo");
    CameraLookTo.KeyCodes = { VK_F2 };

    UInputAction Debug1("Debug1");
    Debug1.KeyCodes = { VK_F1 };

    auto WeakCamera = std::weak_ptr(Camera);
      // 카메라 입력 바인딩 (시스템 컨텍스트 사용)
    UInputManager::Get()->SystemContext->BindAction(CameraUp,
                                                    EKeyEvent::Pressed,
                                                    WeakCamera,
                                                    [this](const FKeyEventData& EventData) {
                                                        Camera->StartMove(Vector3::Forward);
                                                    },
                                                    "CameraMove");

    UInputManager::Get()->SystemContext->BindAction(CameraDown,
                                                    EKeyEvent::Pressed,
                                                    WeakCamera,
                                                    [this](const FKeyEventData& EventData) {
                                                        Camera->StartMove(-Vector3::Forward);
                                                    },
                                                    "CameraMove");

    UInputManager::Get()->SystemContext->BindAction(CameraRight,
                                                    EKeyEvent::Pressed,
                                                    WeakCamera,
                                                    [this](const FKeyEventData& EventData) {
                                                        Camera->StartMove(Vector3::Right);
                                                    },
                                                    "CameraMove");

    UInputManager::Get()->SystemContext->BindAction(CameraLeft,
                                                    EKeyEvent::Pressed,
                                                    WeakCamera,
                                                    [this](const FKeyEventData& EventData) {
                                                        Camera->StartMove(-Vector3::Right);
                                                    },
                                                    "CameraMove");

    UInputManager::Get()->SystemContext->BindAction(CameraFollowObject,
                                                    EKeyEvent::Pressed,
                                                    WeakCamera,
                                                    [this](const FKeyEventData& EventData) {
                                                        Camera->bLookAtObject = !Camera->bLookAtObject;
                                                    },
                                                    "CameraMove");

    UInputManager::Get()->SystemContext->BindAction(CameraLookTo,
                                                    EKeyEvent::Pressed,
                                                    WeakCamera,
                                                    [this](const FKeyEventData& EventData) {
                                                        Camera->LookTo();
                                                    },
                                                    "CameraMove");
}

void UGameplayScene01::SetupBorderTriggers(UElasticBody* InBody)
{
    auto IsInBorder = [this](const Vector3& Position) {
        return std::abs(Position.x) < XBorder &&
            std::abs(Position.y) < YBorder &&
            std::abs(Position.z) < ZBorder;
        };
    
    if (!InBody)
    {
        return;
    }

    InBody->GetRootComp()->OnWorldTransformChangedDelegate.Bind(
        InBody, // 여기서는 객체를 전달해야 함
        [IsInBorder, this, InBody](const FTransform& InTransform) {
            // 약한 참조에서 유효한 공유 포인터를 획득
            if (auto Body = InBody) {
                if (!IsInBorder(InTransform.Position))
                {
                    const Vector3 Position = InTransform.Position;
                    Vector3 Normal = Vector3::Zero;
                    Vector3 NewPosition = Position;

                    // 충돌한 면의 법선 계산과 위치 보정
                    if (std::abs(Position.x) >= XBorder)
                    {
                        Normal.x = Position.x > 0 ? -1.0f : 1.0f;
                        NewPosition.x = XBorder * (Position.x > 0 ? 1.0f : -1.0f);
                    }
                    if (std::abs(Position.y) >= YBorder)
                    {
                        Normal.y = Position.y > 0 ? -1.0f : 1.0f;
                        NewPosition.y = YBorder * (Position.y > 0 ? 1.0f : -1.0f);
                    }
                    if (std::abs(Position.z) >= ZBorder)
                    {
                        Normal.z = Position.z > 0 ? -1.0f : 1.0f;
                        NewPosition.z = ZBorder * (Position.z > 0 ? 1.0f : -1.0f);
                    }

                    Normal.Normalize();

                    // Position correction
                    Body->SetPosition(NewPosition);

                    const Vector3 CurrentVelo = Body->GetCurrentVelocity();
                    const float Restitution = 0.8f;
                    const float VelocityAlongNormal = Vector3::Dot(CurrentVelo, Normal);
                    Vector3 NewImpulse = -(1.0f + Restitution) * VelocityAlongNormal * Normal * Body->GetMass();
                    Body->ApplyImpulse(std::move(NewImpulse));
                }
            }
        },
        "BorderCheck"
    );
}

void UGameplayScene01::SpawnElasticBody()
{
    ////auto body = UGameObject::Create<UElasticBody>();
    auto bodyScoped = ElasticBodyPool->AcquireForcely();
    auto body = bodyScoped.Get();

    body->PostInitialized();
    body->PostInitializedComponents();

    body->SetScale(FRandom::RandF(0.5f, 0.8f) * Vector3::One);
    body->SetPosition(FRandom::RandVector(Vector3::One * -1.5f, Vector3::One * 1.5f));
    body->SetShapeSphere();

    auto Rigid = body->GetComponentByType<URigidBodyComponent>();
    if (Rigid)
    {
        //물리적 상태값 초기화
        Rigid->Reset();
    }
    body->SetMass(FRandom::RandF(1.0f, 5.0f));
    body->SetGravity(bGravity);
    body->SetColor(Vector4(FRandom::RandColor()));
    body->SetActive(true);


    auto Primitive = body->GetComponentByType<UPrimitiveComponent>();
    if (Primitive)
    {
        Primitive->SetMaterial(PoleMaterialHandle);
    }

    SetupBorderTriggers(body);
}

void UGameplayScene01::DeSpawnElasticBody()
{
    //가장 오래된 바디 비활성화    
    auto it = ElasticBodyPool->begin();
    auto weakedBody = *it;

    if (weakedBody.IsValid())
    {
        //객체 비활성화
        weakedBody.Get()->SetActive(false);

        //풀에 반환
        weakedBody.Release();
    }
    
}
