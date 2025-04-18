#pragma once
#include "GameplayScene02.h"
#include "Renderer.h"
#include "RigidBodyComponent.h"
#include "CollisionComponent.h"
#include "CollisionManager.h"
#include "InputManager.h"
#include "Random.h"
#include "define.h"

#include "Debug.h"
#include "ResourceManager.h"
#include "VertexShader.h"
#include "Texture.h"
#include "UIManager.h"
#include"RenderDataTexture.h"
#include "PrimitiveComponent.h"
#include "Material.h"
#include "DebugDrawerManager.h"   
#include "SceneManager.h"
 

UGameplayScene02::UGameplayScene02()
{
    InputContext = UInputContext::Create(SceneName);
}

UGameplayScene02::~UGameplayScene02()
{
    if (Character)
    {
        Character->SetActive(false);
        Character = nullptr;
    }
    if (Character2)
    {
        Character2->SetActive(false);
        Character2 = nullptr;
    }
    if (Camera)
    {
        Camera->SetActive(false);
        Camera = nullptr;
    }
    if (InputContext)
    {
        InputContext = nullptr;
    }
}

void UGameplayScene02::Initialize()
{
    const int VIEW_WIDTH = 800;
    const int VIEW_HEIGHT = 800;

    // 카메라 설정
    Camera = UCamera::Create(PI / 4.0f, VIEW_WIDTH, VIEW_HEIGHT, 0.1f, 100.0f);
    Camera->SetPosition({ 0, 0.0f, -8.0f });

    // 캐릭터 1 (탄성체) 설정
    Character = UGameObject::Create<UElasticBody>();
    Character->SetScale(0.5f * Vector3::One);
    Character->SetPosition({ -0.75f, 0, 0 });
    Character->SetShapeBox();

    // 캐릭터 2 (탄성체) 설정
    Character2 = UGameObject::Create<UElasticBody>();
    Character2->SetScale(0.35f * Vector3::One);
    Character2->SetPosition({ 0.75f, 0, 0 });
    Character2->SetShapeSphere();

    // 초기화 및 설정
    Camera->PostInitialized();
    Character->PostInitialized();
    Character2->PostInitialized();

    Camera->SetLookAtObject(Character.get());
    Camera->LookAt({ 0.0f, 0.0f, 0.0f }); //.정중앙
    Camera->bLookAtObject = false;

    Character->PostInitializedComponents();
    Character2->PostInitializedComponents();
    Camera->PostInitializedComponents();

    //물리속성 설정
    SetMaxSpeeds(MaxSpeed);
    CharacterMass = Character->GetTransform().Scale.Length() * 20.0f;
    Character->SetMass(CharacterMass);

    Character2Mass = Character2->GetTransform().Scale.Length() * 20.0f;
    Character->SetMass(Character2Mass);

    //매터리얼 설정
    auto Primitive1 = Character->GetComponentByType<UPrimitiveComponent>();
    if (Primitive1)
    {
        Primitive1->SetMaterial(TileMaterialHandle);
    }
    auto Primitive2 = Character2->GetComponentByType<UPrimitiveComponent>();
    if (Primitive2)
    {
        Primitive2->SetMaterial(PoleMaterialHandle);
    }

    // 경계 충돌 감지 설정
    SetupBorderTriggers();

    // 입력 설정
    SetupInput();

    // 초기 비활성화
    Character->SetActive(true);
    Character2->SetActive(true);

    Character->SetGravity(false);
    Character2->SetGravity(false);

    auto Colli = Character->GetRootComp()->FindComponentByType<UCollisionComponent>();
    Colli.lock()->OnCollisionEnter.BindSystem([](const FCollisionEventData& InEvent) {
        LOG("CollisionEnter"); }, "OnCollisionEnter_P1");
    Colli.lock()->OnCollisionStay.BindSystem([](const FCollisionEventData& InEvent) {
        LOG("CollisionStay"); }, "OnCollisionEnter_P1");
    Colli.lock()->OnCollisionExit.BindSystem([](const FCollisionEventData& InEvent) {
        LOG("CollisionExit"); }, "OnCollisionEnter_P1");

    //트랜스폼 테스트
    //Character->GetRootComp()->AddChild(Character2->GetRootComp());
}

void UGameplayScene02::Load()
{
    // 입력 컨텍스트 등록
    UInputManager::Get()->RegisterInputContext(InputContext);

    //매터리얼 로드
    TileMaterialHandle = UResourceManager::Get()->LoadResource<UMaterial>(MAT_TILE);
    PoleMaterialHandle = UResourceManager::Get()->LoadResource<UMaterial>(MAT_POLE);
    DefaultMaterialHandle = UResourceManager::Get()->LoadResource<UMaterial>(MAT_DEFAULT);
}

void UGameplayScene02::Unload()
{
    // 주요 객체 해제
    if (Character)
    {
        Character->SetActive(false);
        Character = nullptr;
    }
    if (Character2)
    {
        Character2->SetActive(false);
        Character2 = nullptr;
    }
    if (Camera)
    {
        Camera->SetActive(false);
        Camera = nullptr;
    }

    // 입력 컨텍스트 삭제
    UInputManager::Get()->UnregisterInputContext(SceneName);
}

void UGameplayScene02::Tick(float DeltaTime)
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

}

void UGameplayScene02::SubmitRender(URenderer* Renderer)
{
    if (!Camera)
        return;

    FRenderJob RenderJob1 = Renderer->AllocateRenderJob<FRenderDataTexture>();
    auto Primitive = Character->GetComponentByType<UPrimitiveComponent>();
    if (Primitive)
    {
        if (Primitive->FillRenderData(GetMainCamera(), RenderJob1.RenderData))
        {
            Renderer->SubmitJob(RenderJob1);
        }
    }

    FRenderJob RenderJob2 = Renderer->AllocateRenderJob<FRenderDataTexture>();
    auto Primitive2 = Character2->GetComponentByType<UPrimitiveComponent>();
    if (Primitive2)
    {
        if (Primitive2->FillRenderData(GetMainCamera(), RenderJob2.RenderData))
        {
            Renderer->SubmitJob(RenderJob2);
        }
    }

    float LastDeltaTime = USceneManager::Get()->GetLastTickTime();
    //UDebugDrawManager::Get()->DrawLine(Character->GetTransform().Position,
    //                                   Character2->GetTransform().Position,
    //                                   Vector4(1,1,0,1),
    //                                   0.001f,
    //                                   0.1f);
    UDebugDrawManager::Get()->DrawBox(Character->GetTransform().Position,
                                       Character->GetTransform().Scale * 1.2f,
                                      Character->GetTransform().Rotation,
                                       Vector4(1, 1, 0, 1),
                                      LastDeltaTime,false);
    UDebugDrawManager::Get()->DrawSphere(Character2->GetTransform().Position,
                                         Character2->GetTransform().Scale.x * 0.5f * 1.2f,
                                         Character2->GetTransform().Rotation,
                                         Vector4(0, 1, 1, 1),
                                         LastDeltaTime, false);

}

void UGameplayScene02::SubmitRenderUI()
{
    const ImGuiWindowFlags UIWindowFlags =
        ImGuiWindowFlags_NoResize |      // 크기 조절 비활성화
        ImGuiWindowFlags_AlwaysAutoResize;  // 항상 내용에 맞게 크기 조절

    UUIManager::Get()->RegisterUIElement("Scene02UI", [this]() {

        if (Camera)
        {
            ImGui::Begin("Camera", nullptr, UIWindowFlags);
            ImGui::Checkbox("bIs2D", &Camera->bIs2D);
            ImGui::Checkbox("bLookAtObject", &Camera->bLookAtObject);
            ImGui::Text(Debug::ToString(Camera->GetTransform()));
            ImGui::End();
        }

        if (Character)
        {
            Vector3 CurrentVelo = Character->GetCurrentVelocity();
            Vector3 CurrentPos = Character->GetTransform().Position;
            bool bGravity = Character->IsGravity();
            bool bPhysics = Character->IsPhysicsSimulated();
            ImGui::Begin("Charcter", nullptr, UIWindowFlags);
            ImGui::Checkbox("bIsMove", &Character->bIsMoving);
            ImGui::Checkbox("bPhysicsBased", &bPhysics);
            ImGui::Checkbox("bGravity", &bGravity);
            Character->SetGravity(bGravity);
            Character->SetPhysics(bPhysics);
            ImGui::Text("CurrentVelo : %.2f  %.2f  %.2f", CurrentVelo.x,
                        CurrentVelo.y,
                        CurrentVelo.z);
            ImGui::Text("Position : %.2f  %.2f  %.2f", CurrentPos.x,
                        CurrentPos.y,
                        CurrentPos.z);
            auto Colli = Character->GetComponentByType<UCollisionComponent>();
            if (Colli)
            {
                Vector3 ColliWorldPos = Colli->GetWorldPosition();
                Vector3 ColliLocalPos = Colli->GetLocalPosition();
                ImGui::Text("ColliWorldPosition : %.2f  %.2f  %.2f", ColliWorldPos.x,
                            ColliWorldPos.y,
                            ColliWorldPos.z);
                ImGui::Text("ColliLocalPosition : %.2f  %.2f  %.2f", ColliLocalPos.x,
                            ColliLocalPos.y,
                            ColliLocalPos.z);
            }

            if (ImGui::Button("CompTree")) 
            {
                Character->GetRootComp()->PrintComponentTree();
            }
            ImGui::End();

        }

        if (Character2)
        {
            Vector3 CurrentPos = Character2->GetTransform().Position;
            Quaternion CurrentRot = Character2->GetTransform().Rotation;
            Vector3 CurrentVelo = Character2->GetCurrentVelocity();
            bool bGravity2 = Character2->IsGravity();
            bool bPhysics2 = Character2->IsPhysicsSimulated();
            bool bIsActive2 = Character2->IsActive();
            ImGui::Begin("Charcter2", nullptr, UIWindowFlags);
            ImGui::Checkbox("bIsMove", &Character2->bIsMoving);
            ImGui::Checkbox("bPhysicsBased", &bPhysics2);
            ImGui::Checkbox("bGravity", &bGravity2);
            Character2->SetGravity(bGravity2);
            Character2->SetPhysics(bPhysics2);
            ImGui::Text("CurrentVelo : %.2f  %.2f  %.2f", CurrentVelo.x,
                        CurrentVelo.y,
                        CurrentVelo.z);
            ImGui::Text("Position : %.2f  %.2f  %.2f", CurrentPos.x,
                        CurrentPos.y,
                        CurrentPos.z);
            ImGui::Text("Rot : %.2f  %.2f  %.2f  %.2f", CurrentRot.x,
                        CurrentRot.y,
                        CurrentRot.z, 
                        CurrentRot.w);
            ImGui::End();
        }

        ImGui::Begin("Both", nullptr, UIWindowFlags);
        ImGui::SetNextItemWidth(50.0f);
        if (ImGui::InputFloat("MaxSpeed", &MaxSpeed, 0.0f, 0.0f, "%.02f"))
        {
            SetMaxSpeeds(MaxSpeed);
        }
        ImGui::SetNextItemWidth(50.0f);
        ImGui::InputFloat("PowerMags", &PowerMagnitude, 0.0f, 0.0f, "%.02f");
        ImGui::End();
        });
}

void UGameplayScene02::HandleInput(const FKeyEventData& EventData)
{
    // 이 함수는 사용되지 않을 수 있음 - InputContext가 대부분의 입력 처리를 담당
    // 필요한 경우 직접적인 입력 처리를 여기에 추가
}

inline UCamera* UGameplayScene02::GetMainCamera() const 
{
    return Camera.get();
}

void UGameplayScene02::SetupInput()
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

    auto WeakCharacter = std::weak_ptr(Character);
    // 캐릭터 1 입력 바인딩
    InputContext->BindAction(AMoveUp_P1,
                             EKeyEvent::Pressed,
                             WeakCharacter,
                             [this](const FKeyEventData& EventData) {
                                 float InForceMagnitude = CharacterMass * PowerMagnitude;

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
                             WeakCharacter,
                             [this](const FKeyEventData& EventData) {
                                 float InForceMagnitude = CharacterMass * PowerMagnitude;

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
                             WeakCharacter,
                             [this](const FKeyEventData& EventData) {
                                 float InForceMagnitude = CharacterMass * PowerMagnitude;

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
                             WeakCharacter,
                             [this](const FKeyEventData& EventData) {
                                 float InForceMagnitude = CharacterMass * PowerMagnitude;

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
                             WeakCharacter,
                             [this](const FKeyEventData& EventData) {
                                 Character->StopMove();
                             },
                             "CharacterMove");

    auto WeakCharacter2 = std::weak_ptr(Character2);
      // 캐릭터 2 입력 바인딩
    InputContext->BindAction(AMoveUp_P2,
                             EKeyEvent::Pressed,
                             WeakCharacter2,
                             [this](const FKeyEventData& EventData) {
                                 float InForceMagnitude = Character2Mass * PowerMagnitude;

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
                             WeakCharacter2,
                             [this](const FKeyEventData& EventData) {
                                 float InForceMagnitude = Character2Mass * PowerMagnitude;

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
                             WeakCharacter2,
                             [this](const FKeyEventData& EventData) {
                                 float InForceMagnitude = Character2Mass * PowerMagnitude;

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
                             WeakCharacter2,
                             [this](const FKeyEventData& EventData) {
                                 float InForceMagnitude = Character2Mass * PowerMagnitude;

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
                             WeakCharacter2,
                             [this](const FKeyEventData& EventData) {
                                 Character2->StopMove();
                             },
                             "CharacterMove");

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

      // 디버그 입력 바인딩
    UInputManager::Get()->SystemContext->BindActionSystem(Debug1,
                                                          EKeyEvent::Pressed,
                                                          [this](const FKeyEventData& EventData) {
                                                              if (Character2.get())
                                                              {
                                                                  Vector3 TargetPos = Character2->GetTransform().Position;
                                                                  TargetPos += Vector3::Right * 0.15f;
                                                                  TargetPos += Vector3::Up * 0.15f;
                                                                  Character2->GetRootComp()->FindChildByType<URigidBodyComponent>().lock()->ApplyImpulse(
                                                                      Vector3::Right * 1.0f,
                                                                      TargetPos);
                                                              }
                                                          },
                                                          "DebugAction");
}

void UGameplayScene02::SetupBorderTriggers()
{
    auto IsInBorder = [this](const Vector3& Position) {
        return std::abs(Position.x) < XBorder &&
            std::abs(Position.y) < YBorder &&
            std::abs(Position.z) < ZBorder;
        };

    Character->GetRootComp()->OnWorldTransformChangedDelegate.Bind(Character.get(),
                                                               [IsInBorder, this](const FTransform& InTransform) {
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
                                                                       //Position correction
                                                                       Character->SetPosition(NewPosition);

                                                                       const Vector3 CurrentVelo = Character->GetCurrentVelocity();
                                                                       const float Restitution = 1.0f;
                                                                       const float VelocityAlongNormal = Vector3::Dot(CurrentVelo, Normal);
                                                                       Vector3 NewImpulse = -(1.0f + Restitution) * VelocityAlongNormal * Normal * Character->GetMass();

                                                                       Character->ApplyImpulse(std::move(NewImpulse));
                                                                   }
                                                               },
                                                               "BorderCheck");
    auto weakChacter2 = std::weak_ptr(Character2);
    Character2->GetRootComp()->OnWorldTransformChangedDelegate.Bind(Character.get(),
                                                                [IsInBorder, this](const FTransform& InTransform) {
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
                                                                        //Position correction
                                                                        Character2->SetPosition(NewPosition);

                                                                        const Vector3 CurrentVelo = Character2->GetCurrentVelocity();
                                                                        const float Restitution = 1.0f;
                                                                        const float VelocityAlongNormal = Vector3::Dot(CurrentVelo, Normal);
                                                                        Vector3 NewImpulse = -(1.0f + Restitution) * VelocityAlongNormal * Normal * Character2->GetMass();
                                                                        Character2->ApplyImpulse(std::move(NewImpulse));
                                                                    }
                                                                },
                                                                "BorderCheck");
}

void UGameplayScene02::SetMaxSpeeds(const float InMaxSpeed)
{
    MaxSpeed = InMaxSpeed;
    if (Character)
    {
        Character->SetMaxSpeed(MaxSpeed);
    }
    if (Character2)
    {
        Character2->SetMaxSpeed(MaxSpeed);
    }
}

void UGameplayScene02::SetPowerMagnitude(const float InMagnitude)
{
    PowerMagnitude = InMagnitude;
}
