#pragma once
#include "GameplayScene01.h"
#include "Renderer.h"
#include "RigidBodyComponent.h"
#include "CollisionComponent.h"
#include "PrimitiveComponent.h"
#include "ConfigReadManager.h"

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
#include <new>

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
    int VIEW_WIDTH, VIEW_HEIGHT;
    UConfigReadManager::Get()->GetValue("ScreenWidth", VIEW_WIDTH);
    UConfigReadManager::Get()->GetValue("ScreenHeight", VIEW_HEIGHT);

    // 카메라 설정
    Camera = UCamera::Create(PI / 4.0f, VIEW_WIDTH, VIEW_HEIGHT, 0.1f, 5000.0f);
    Camera->SetPosition({ 0, 0.0f, -800.0f });
    Camera->PostInitialized();
    Camera->PostInitializedComponents();

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

    ElasticBodyPool = std::make_unique< TFixedObjectPool<UElasticBody, 512>>(EElasticBodyShape::Sphere);
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
                    Vector3 Normal = Vector3::Zero();
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

    if (ElasticBodyPool->GetActiveCount() % 2 == 0)
    {
        new (body) UElasticBody(EElasticBodyShape::Box);
    }
    else
    {
        new (body) UElasticBody(EElasticBodyShape::Sphere);
    }
    body->PostInitialized();
    body->PostInitializedComponents();

    body->SetScale(FRandom::RandVector(Vector3::One()*30.0f , Vector3::One()*80.0f));
    body->SetPosition(FRandom::RandVector(Vector3::One() * -150.0f, Vector3::One() * 150.0f));

    auto Rigid = body->GetComponentByType<URigidBodyComponent>();
    if (Rigid)
    {
        //물리적 상태값 초기화
        Rigid->Reset();
    }
    body->SetMass(FRandom::RandF(1.0f, 20.0f));
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

Vector3 UGameplayScene01::CalculateSphericPosition(float Latidue, float Longitude, float radius)
{
    // 위도와 경도를 라디안으로 변환 (입력이 도(degree) 단위라고 가정)
    float latRad = Math::DegreeToRad(Latidue);      // 위도
    float lonRad = Math::DegreeToRad(Longitude);     // 경도

    // x, y, z 좌표 계산
    float x = radius * cos(latRad) * sin(lonRad);   // x 좌표
    float y = radius * sin(latRad);                 // y 좌표 (위도에 따라 높이)
    float z = -radius * cos(latRad) * cos(lonRad);   // z 좌표

    return Vector3(x, y, z);
}
