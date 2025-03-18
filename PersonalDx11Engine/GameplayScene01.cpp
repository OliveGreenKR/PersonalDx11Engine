#pragma once
#include "GameplayScene01.h"
#include "Renderer.h"
#include "RigidBodyComponent.h"
#include "CollisionComponent.h"
#include "CollisionManager.h"
#include "InputManager.h"
#include "Random.h"
#include "DebugDrawManager.h"
#include "define.h"
#include "Color.h"
#include "Debug.h"
#include "ResourceManager.h"
#include "Texture.h"
#include "UIManager.h"

UGameplayScene01::UGameplayScene01()
{
    InputContext = UInputContext::Create(SceneName);
}

void UGameplayScene01::Initialize()
{
    // 모델 가져오기
    auto CubeModel = UModelBufferManager::Get()->GetCubeModel();
    auto SphereModel = UModelBufferManager::Get()->GetSphereModel();

    const int VIEW_WIDTH = 800;
    const int VIEW_HEIGHT = 800;

    // 카메라 설정
    Camera = UCamera::Create(PI / 4.0f, VIEW_WIDTH, VIEW_HEIGHT, 0.1f, 100.0f);
    Camera->SetPosition({ 0, 0.0f, -10.0f });
   
    // 입력 설정
    SetupInput();

    // 입력 컨텍스트 등록
    UInputManager::Get()->RegisterInputContext(InputContext);
}

void UGameplayScene01::Load()
{
    // 텍스처 로드
    TextureTile = UResourceManager::Get()->LoadTexture(TEXTURE03);
	TexturePole = UResourceManager::Get()->LoadTexture(TEXTURE02);
}

void UGameplayScene01::Unload()
{
    // 입력 컨텍스트 삭제
    UInputManager::Get()->UnregisterInputContext(SceneName);

    // 모든 활성 객체 해제
    for (auto body : ElasticBodies)
    {
        body->SetActive(false);
    }
    ElasticBodies.clear();

    // 주요 객체 해제
    Camera = nullptr;

    // 텍스처 해제
    TextureTile = nullptr;
    TexturePole = nullptr;
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
    for (auto& elasticBody : ElasticBodies)
    {
        elasticBody->Tick(DeltaTime);
    }

}

void UGameplayScene01::SubmitRender(URenderer* Renderer)
{
    for (auto ebody : ElasticBodies)
    {
        Renderer->SubmitRenderJobsInObject(Camera.get(), ebody.get(), TexturePole.get()->GetShaderResourceView());
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
            LOG("%s", bGravity ? "T" : "F");
            for (auto ebody : ElasticBodies)
            {
                ebody->SetGravity(bGravity);
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

      // 카메라 입력 바인딩 (시스템 컨텍스트 사용)
    UInputManager::Get()->SystemContext->BindAction(CameraUp,
                                                    EKeyEvent::Pressed,
                                                    Camera,
                                                    [this](const FKeyEventData& EventData) {
                                                        Camera->StartMove(Vector3::Forward);
                                                    },
                                                    "CameraMove");

    UInputManager::Get()->SystemContext->BindAction(CameraDown,
                                                    EKeyEvent::Pressed,
                                                    Camera,
                                                    [this](const FKeyEventData& EventData) {
                                                        Camera->StartMove(-Vector3::Forward);
                                                    },
                                                    "CameraMove");

    UInputManager::Get()->SystemContext->BindAction(CameraRight,
                                                    EKeyEvent::Pressed,
                                                    Camera,
                                                    [this](const FKeyEventData& EventData) {
                                                        Camera->StartMove(Vector3::Right);
                                                    },
                                                    "CameraMove");

    UInputManager::Get()->SystemContext->BindAction(CameraLeft,
                                                    EKeyEvent::Pressed,
                                                    Camera,
                                                    [this](const FKeyEventData& EventData) {
                                                        Camera->StartMove(-Vector3::Right);
                                                    },
                                                    "CameraMove");

    UInputManager::Get()->SystemContext->BindAction(CameraFollowObject,
                                                    EKeyEvent::Pressed,
                                                    Camera,
                                                    [this](const FKeyEventData& EventData) {
                                                        Camera->bLookAtObject = !Camera->bLookAtObject;
                                                    },
                                                    "CameraMove");

    UInputManager::Get()->SystemContext->BindAction(CameraLookTo,
                                                    EKeyEvent::Pressed,
                                                    Camera,
                                                    [this](const FKeyEventData& EventData) {
                                                        Camera->LookTo();
                                                    },
                                                    "CameraMove");
}

void UGameplayScene01::SetupBorderTriggers(shared_ptr<UElasticBody>& InBody)
{
    auto IsInBorder = [this](const Vector3& Position) {
        return std::abs(Position.x) < XBorder &&
            std::abs(Position.y) < YBorder &&
            std::abs(Position.z) < ZBorder;
        };

    if (!InBody.get())
        return;

    // 약한 참조를 사용하여 순환 참조 방지
    std::weak_ptr<UElasticBody> WeakBody = InBody;

    InBody->GetRootComp()->OnWorldTransformChangedDelegate.Bind(
        InBody, // 여기서는 객체를 전달해야 함
        [IsInBorder, this, WeakBody](const FTransform& InTransform) {
            // 약한 참조에서 유효한 공유 포인터를 획득
            if (auto Body = WeakBody.lock()) {
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
    auto body = UGameObject::Create<UElasticBody>();
    body->PostInitialized();
    body->PostInitializedComponents();

    body->SetScale(FRandom::RandF(0.5f, 0.8f) * Vector3::One);
    body->SetPosition(FRandom::RandVector(Vector3::One * -1.5f, Vector3::One * 1.5f));
    body->SetShapeSphere();

    body->SetMass(FRandom::RandF(1.0f, 5.0f));
    body->SetGravity(bGravity);

    body->SetActive(true);

    SetupBorderTriggers(body);

    ElasticBodies.push_back(body);
    LOG("ElasticBody Count : %03d", ElasticBodies.size());
}
