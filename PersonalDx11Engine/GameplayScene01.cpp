#pragma once
#include "GameplayScene01.h"
#include "Renderer.h"
#include "RigidBodyComponent.h"
#include "CollisionComponent.h"
#include "CollisionManager.h"
#include "InputManager.h"
#include "Random.h"
#include "DebugDrawManager.h"
#include "RscUtil.h"
#include "define.h"
#include "Color.h"
#include "Debug.h"
#include "ResourceManager.h"
#include "Texture.h"
#include "UIManager.h"

UGameplayScene01::UGameplayScene01()
{
    InputContext = UInputContext::Create("Scene01");
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

    // 캐릭터 1 (큐브) 설정
    Character = UGameObject::Create<UGameObject>(CubeModel);
    Character->SetScale(0.25f * Vector3::One);
    Character->SetPosition({ 0, 0, 0 });
    Character->bDebug = true;

    auto RigidComp1 = UActorComponent::Create<URigidBodyComponent>();
    RigidComp1->SetMass(CharacterMass);

    auto CollisionComp1 = UActorComponent::Create<UCollisionComponent>();
    CollisionComp1->SetShapeBox();
    CollisionComp1->SetHalfExtent(0.5f * Character->GetTransform()->GetScale());
    CollisionComp1->BindRigidBody(RigidComp1);
    CollisionComp1->SetActive(false);

    Character->AddActorComponent(RigidComp1);

    // 캐릭터 2 (탄성체) 설정
    Character2 = UGameObject::Create<UElasticBody>();
    Character2->SetScale(0.75f * Vector3::One);
    Character2->SetPosition({ 1, 0, 0 });
    Character2->SetShapeSphere();
    Character2->bDebug = true;

    // 초기화 및 설정
    Camera->PostInitialized();
    Character->PostInitialized();
    Character2->PostInitialized();

    Camera->SetLookAtObject(Character.get());
    Camera->LookTo(Character->GetTransform()->GetPosition());
    Camera->bLookAtObject = false;

    Character->PostInitializedComponents();
    Character2->PostInitializedComponents();
    Camera->PostInitializedComponents();

    // 경계 충돌 감지 설정
    SetupBorderTriggers();

    // 입력 설정
    SetupInput();

    // 초기 비활성화
    Character->SetActive(false);
    Character2->SetActive(false);

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
    UInputManager::Get()->UnregisterInputContext("Scene01");

    // 모든 활성 객체 삭제
    ElasticBodies.clear();

    // 주요 객체 해제
    Character = nullptr;
    Character2 = nullptr;
    Camera = nullptr;

    // 텍스처 해제
    TextureTile = nullptr;
    TexturePole = nullptr;
}

void UGameplayScene01::Tick(float DeltaTime)
{
    // 주요 객체 업데이트
    if (Character)
    {
        Character->Tick(DeltaTime);
    }

    if (Character2)
    {
        Character2->Tick(DeltaTime);
    }

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
    Renderer->SubmitRenderJob(Camera.get(), Character.get(), TextureTile.get()->GetShaderResourceView());
    Renderer->SubmitRenderJob(Camera.get(), Character2.get(), TexturePole.get()->GetShaderResourceView());

    for (auto ebody : ElasticBodies)
    {
        Renderer->SubmitRenderJob(Camera.get(), ebody.get(), TexturePole.get()->GetShaderResourceView());
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
            ImGui::Text(Debug::ToString(*Camera->GetTransform()));
            ImGui::End();
        }

        ImGui::Begin("ElasticBodies", nullptr, UIWindowFlags);
        //ImGui::InputInt("##Value", &value); // 숫자 입력 필드
        //ImGui::Text("%d / %d",
        //			UElasticBodyManager::Get()->GetActiveBodyCount(),
        //			UElasticBodyManager::Get()->GetPooledBodyCount());
        //ImGui::SameLine();
        //if (ImGui::Button("-")) {
        //	UElasticBodyManager::Get()->DespawnRandomBody();
        //	UCollisionManager::Get()->PrintTreeStructure();
        //}
        //ImGui::SameLine();
        //if (ImGui::Button("+")) {
        //	UElasticBodyManager::Get()->SpawnRandomBody();
        //	UCollisionManager::Get()->PrintTreeStructure();
        //}
        ImGui::SameLine();
        ImGui::Checkbox("bSpawnBody", &bSpawnBody);
        if (ImGui::Button("Print")) {
            UCollisionManager::Get()->PrintTreeStructure();
        }
        ImGui::End();

        if (Character)
        {
            Vector3 CurrentVelo = Character->GetCurrentVelocity();
            bool bGravity = Character->IsGravity();
            bool bPhysics = Character->IsPhysicsSimulated();
            ImGui::Begin("Charcter", nullptr, UIWindowFlags);
            ImGui::Checkbox("bIsMove", &Character->bIsMoving);
            ImGui::Checkbox("bDebug", &Character->bDebug);
            ImGui::Checkbox("bPhysicsBased", &bPhysics);
            ImGui::Checkbox("bGravity", &bGravity);
            Character->SetGravity(bGravity);
            Character->SetPhysics(bPhysics);
            ImGui::Text("CurrentVelo : %.2f  %.2f  %.2f", CurrentVelo.x,
                        CurrentVelo.y,
                        CurrentVelo.z);
            ImGui::End();
        }

        if (Character2)
        {
            Vector3 CurrentVelo = Character2->GetCurrentVelocity();
            bool bGravity2 = Character2->IsGravity();
            bool bPhysics2 = Character2->IsPhysicsSimulated();
            bool bIsActive2 = Character2->IsActive();
            ImGui::Begin("Charcter2", nullptr, UIWindowFlags);
            ImGui::Checkbox("bIsMove", &Character2->bIsMoving);
            ImGui::Checkbox("bDebug", &Character2->bDebug);
            ImGui::Checkbox("bPhysicsBased", &bPhysics2);
            ImGui::Checkbox("bGravity", &bGravity2);
            Character2->SetGravity(bGravity2);
            Character2->SetPhysics(bPhysics2);
            ImGui::Text("CurrentVelo : %.2f  %.2f  %.2f", CurrentVelo.x,
                        CurrentVelo.y,
                        CurrentVelo.z);
            ImGui::Text("Position : %.2f  %.2f  %.2f", Character2->GetTransform()->GetPosition().x,
                        Character2->GetTransform()->GetPosition().y,
                        Character2->GetTransform()->GetPosition().z);
            ImGui::Text("Rotation : %.2f  %.2f  %.2f", Character2->GetTransform()->GetEulerRotation().x,
                        Character2->GetTransform()->GetEulerRotation().y,
                        Character2->GetTransform()->GetEulerRotation().z);

            ImGui::End();
        }


                                           });
}

void UGameplayScene01::HandleInput(const FKeyEventData& EventData)
{
    // 이 함수는 사용되지 않을 수 있음 - InputContext가 대부분의 입력 처리를 담당
    // 필요한 경우 직접적인 입력 처리를 여기에 추가
}

void UGameplayScene01::SetupInput()
{
    // 입력 액션 정의
    UInputAction AMoveUp_P1("AMoveUp_P1");
    AMoveUp_P1.KeyCodes = { 'W' };

    UInputAction AMoveDown_P1("AMoveDown_P1");
    AMoveDown_P1.KeyCodes = { 'S' };

    UInputAction AMoveRight_P1("AMoveRight_P1");
    AMoveRight_P1.KeyCodes = { 'D' };

    UInputAction AMoveLeft_P1("AMoveLeft_P1");
    AMoveLeft_P1.KeyCodes = { 'A' };

    UInputAction AMoveStop_P1("AMoveStop_P1");
    AMoveStop_P1.KeyCodes = { 'F' };

    UInputAction AMoveUp_P2("AMoveUp_P2");
    AMoveUp_P2.KeyCodes = { 'I' };

    UInputAction AMoveDown_P2("AMoveDown_P2");
    AMoveDown_P2.KeyCodes = { 'K' };

    UInputAction AMoveRight_P2("AMoveRight_P2");
    AMoveRight_P2.KeyCodes = { 'L' };

    UInputAction AMoveLeft_P2("AMoveLeft_P2");
    AMoveLeft_P2.KeyCodes = { 'J' };

    UInputAction AMoveStop_P2("AMoveStop_P2");
    AMoveStop_P2.KeyCodes = { 'H' };

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

    // 캐릭터 1 입력 바인딩
    InputContext->BindAction(AMoveUp_P1,
                             EKeyEvent::Pressed,
                             Character,
                             [this](const FKeyEventData& EventData) {
                                 float InForceMagnitude = CharacterMass * 100.0f;

                                 if (EventData.bShift)
                                 {
                                     Character->StartMove(Vector3::Forward);
                                 }
                                 else
                                 {
                                     Character->ApplyForce(Vector3::Forward * InForceMagnitude);
                                 }
                             },
                             "CharacterMove");

    InputContext->BindAction(AMoveDown_P1,
                             EKeyEvent::Pressed,
                             Character,
                             [this](const FKeyEventData& EventData) {
                                 float InForceMagnitude = CharacterMass * 100.0f;

                                 if (EventData.bShift)
                                 {
                                     Character->StartMove(-Vector3::Forward);
                                 }
                                 else
                                 {
                                     Character->ApplyForce(-Vector3::Forward * InForceMagnitude);
                                 }
                             },
                             "CharacterMove");

    InputContext->BindAction(AMoveRight_P1,
                             EKeyEvent::Pressed,
                             Character,
                             [this](const FKeyEventData& EventData) {
                                 float InForceMagnitude = CharacterMass * 100.0f;

                                 if (EventData.bShift)
                                 {
                                     Character->StartMove(Vector3::Right);
                                 }
                                 else
                                 {
                                     Character->ApplyForce(Vector3::Right * InForceMagnitude);
                                 }
                             },
                             "CharacterMove");

    InputContext->BindAction(AMoveLeft_P1,
                             EKeyEvent::Pressed,
                             Character,
                             [this](const FKeyEventData& EventData) {
                                 float InForceMagnitude = CharacterMass * 100.0f;

                                 if (EventData.bShift)
                                 {
                                     Character->StartMove(-Vector3::Right);
                                 }
                                 else
                                 {
                                     Character->ApplyForce(-Vector3::Right * InForceMagnitude);
                                 }
                             },
                             "CharacterMove");

    InputContext->BindAction(AMoveStop_P1,
                             EKeyEvent::Pressed,
                             Character,
                             [this](const FKeyEventData& EventData) {
                                 Character->StopMove();
                             },
                             "CharacterMove");

      // 캐릭터 2 입력 바인딩
    InputContext->BindAction(AMoveUp_P2,
                             EKeyEvent::Pressed,
                             Character2,
                             [this](const FKeyEventData& EventData) {
                                 float InForceMagnitude = Character2Mass * 100.0f;

                                 if (EventData.bShift)
                                 {
                                     Character2->StartMove(Vector3::Forward);
                                 }
                                 else
                                 {
                                     Character2->ApplyForce(Vector3::Forward * InForceMagnitude);
                                 }
                             },
                             "CharacterMove");

    InputContext->BindAction(AMoveDown_P2,
                             EKeyEvent::Pressed,
                             Character2,
                             [this](const FKeyEventData& EventData) {
                                 float InForceMagnitude = Character2Mass * 100.0f;

                                 if (EventData.bShift)
                                 {
                                     Character2->StartMove(-Vector3::Forward);
                                 }
                                 else
                                 {
                                     Character2->ApplyForce(-Vector3::Forward * InForceMagnitude);
                                 }
                             },
                             "CharacterMove");

    InputContext->BindAction(AMoveRight_P2,
                             EKeyEvent::Pressed,
                             Character2,
                             [this](const FKeyEventData& EventData) {
                                 float InForceMagnitude = Character2Mass * 100.0f;

                                 if (EventData.bShift)
                                 {
                                     Character2->StartMove(Vector3::Right);
                                 }
                                 else
                                 {
                                     Character2->ApplyForce(Vector3::Right * InForceMagnitude);
                                 }
                             },
                             "CharacterMove");

    InputContext->BindAction(AMoveLeft_P2,
                             EKeyEvent::Pressed,
                             Character2,
                             [this](const FKeyEventData& EventData) {
                                 float InForceMagnitude = Character2Mass * 100.0f;

                                 if (EventData.bShift)
                                 {
                                     Character2->StartMove(-Vector3::Right);
                                 }
                                 else
                                 {
                                     Character2->ApplyForce(-Vector3::Right * InForceMagnitude);
                                 }
                             },
                             "CharacterMove");

    InputContext->BindAction(AMoveStop_P2,
                             EKeyEvent::Pressed,
                             Character2,
                             [this](const FKeyEventData& EventData) {
                                 Character2->StopMove();
                             },
                             "CharacterMove");

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

      // 디버그 입력 바인딩
    UInputManager::Get()->SystemContext->BindActionSystem(Debug1,
                                                          EKeyEvent::Pressed,
                                                          [this](const FKeyEventData& EventData) {
                                                              if (Character2.get())
                                                              {
                                                                  Vector3 TargetPos = Character2->GetTransform()->GetPosition();
                                                                  TargetPos += Vector3::Right * 0.15f;
                                                                  TargetPos += Vector3::Up * 0.15f;
                                                                  Character2->GetRootActorComp()->FindChildByType<URigidBodyComponent>()->ApplyImpulse(
                                                                      Vector3::Right * 1.0f,
                                                                      TargetPos);
                                                              }
                                                          },
                                                          "DebugAction");
}

void UGameplayScene01::SetupBorderTriggers()
{
    auto IsInBorder = [this](const Vector3& Position) {
        return std::abs(Position.x) < XBorder &&
            std::abs(Position.y) < YBorder &&
            std::abs(Position.z) < ZBorder;
        };

    Character->GetTransform()->OnTransformChangedDelegate.Bind(Character,
                                                               [IsInBorder, this](const FTransform& InTransform) {
                                                                   if (!IsInBorder(InTransform.GetPosition()))
                                                                   {
                                                                       const Vector3 Position = InTransform.GetPosition();
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
                                                                       //Position correction
                                                                       Character->GetTransform()->SetPosition(NewPosition);

                                                                       const Vector3 CurrentVelo = Character->GetCurrentVelocity();
                                                                       const float Restitution = 0.8f;
                                                                       const float VelocityAlongNormal = Vector3::Dot(CurrentVelo, Normal);
                                                                       Vector3 NewImpulse = -(1.0f + Restitution) * VelocityAlongNormal * Normal * Character->GetMass();

                                                                       Character->ApplyImpulse(std::move(NewImpulse));
                                                                   }
                                                               },
                                                               "BorderCheck");

    Character2->GetTransform()->OnTransformChangedDelegate.Bind(Character2,
                                                                [IsInBorder, this](const FTransform& InTransform) {
                                                                    if (!IsInBorder(InTransform.GetPosition()))
                                                                    {
                                                                        const Vector3 Position = InTransform.GetPosition();
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
                                                                        //Position correction
                                                                        Character2->GetTransform()->SetPosition(NewPosition);

                                                                        const Vector3 CurrentVelo = Character2->GetCurrentVelocity();
                                                                        const float Restitution = 0.8f;
                                                                        const float VelocityAlongNormal = Vector3::Dot(CurrentVelo, Normal);
                                                                        Vector3 NewImpulse = -(1.0f + Restitution) * VelocityAlongNormal * Normal * Character2->GetMass();

                                                                        Character2->ApplyImpulse(std::move(NewImpulse));
                                                                    }
                                                                },
                                                                "BorderCheck");
}

void UGameplayScene01::SpawnElasticBody()
{
    auto tmpBody = UGameObject::Create<UElasticBody>();
    tmpBody->SetScale(FRandom::RandF(0.5f, 0.8f) * Vector3::One);
    tmpBody->SetPosition(FRandom::RandVector(Vector3::One * -1.5f, Vector3::One * 1.5f));
    tmpBody->SetMass(FRandom::RandF(1.0f, 5.0f));
    tmpBody->SetShapeSphere();
    tmpBody->bDebug = true;
    tmpBody->SetDebugColor((Vector4)FRandom::RandVector({ 0,0,0 }, { 1,1,1 }));
    tmpBody->PostInitialized();
    tmpBody->PostInitializedComponents();
    tmpBody->SetActive(true);

    ElasticBodies.push_back(tmpBody);
    LOG("ElasticBody Count : %03d", ElasticBodies.size());
}
