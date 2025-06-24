#pragma once
#include "GameplayScene02.h"
#include "Renderer.h"
#include "RigidBodyComponent.h"
#include "CollisionComponent.h"
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
#include "ConfigReadManager.h"
#include "UI_DragVector.h"

UGameplayScene02::UGameplayScene02()
{
    InputContext = UInputContext::Create(SceneName.c_str());
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
    LOG("[SCENE] Scene02 Init");
    int VIEW_WIDTH, VIEW_HEIGHT;
    UConfigReadManager::Get()->GetValue("ScreenWidth", VIEW_WIDTH);
    UConfigReadManager::Get()->GetValue("ScreenHeight", VIEW_HEIGHT);
    UConfigReadManager::Get()->GetValue("Scene02MaxSpeed", MaxSpeed);
    UConfigReadManager::Get()->GetValue("Scene02PowerMagnitude", PowerMagnitude);
    UConfigReadManager::Get()->GetValue("Scene02CharacterMass", CharacterMass);
    UConfigReadManager::Get()->GetValue("Scene02Character2Mass", Character2Mass);

    // 카메라 설정
    Camera = UCamera::Create(PI / 4.0f, VIEW_WIDTH, VIEW_HEIGHT, 0.1f, 5000.0f);
    Camera->SetPosition({ 0, 0.0f, -800.0f });
    Camera->SetMovementSpeed(100.0f);

    // 캐릭터 1 (탄성체) 설정
    Character = UGameObject::Create<UElasticBody>(EElasticBodyShape::Box);
    Character->SetScale(40.0f * Vector3::One());
    Character->SetPosition({ -60.0f, 0, 0.0f });

    // 캐릭터 2 (탄성체) 설정
    Character2 = UGameObject::Create<UElasticBody>(EElasticBodyShape::Box);
    Character2->SetScale(50.0f * Vector3::One());
    Character2->SetPosition({ -60.0f, -60.0f, 0.0f });

    // 초기화 및 설정
    Camera->PostInitialized();
    Character->PostInitialized();
    Character2->PostInitialized();

    Camera->SetLookAtObject(Character.get());
    Camera->LookAt({ 0.0f, 0.0f, 0.0f }); //정중앙
    Camera->bLookAtObject = false;

    Character->PostInitializedComponents();
    Character2->PostInitializedComponents();
    Camera->PostInitializedComponents();

    //물리속성 설정
    SetMaxSpeeds(MaxSpeed);
    Character->SetMass(CharacterMass);
    Character->SetRestitution(0.80f);

    Character2->SetMass(Character2Mass);
    Character2->SetRestitution(0.80f);


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

    Character->SetGravity(false);
    Character2->SetGravity(false);

    auto Colli = Character->GetRootComp()->FindComponentByType<UCollisionComponentBase>();
    Colli.lock()->OnCollisionEnter.BindSystem([](const FCollisionEventData& InEvent) {
        LOG("CollisionEnter"); }, "OnCollisionEnter_P1");
    Colli.lock()->OnCollisionStay.BindSystem([](const FCollisionEventData& InEvent) {
        LOG("CollisionStay"); }, "OnCollisionEnter_P1");
    Colli.lock()->OnCollisionExit.BindSystem([](const FCollisionEventData& InEvent) {
        LOG("CollisionExit"); }, "OnCollisionEnter_P1");
    Colli.lock()->SetLocalScale(Vector3::One() * 1.05f);

    auto Colli2 = Character2->GetRootComp()->FindComponentByType<UCollisionComponentBase>();
    Colli2.lock()->SetLocalScale(Vector3::One() * 1.05f);
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
    UInputManager::Get()->UnregisterInputContext(SceneName.c_str());
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
}

void UGameplayScene02::SubmitRenderUI()
{
    static bool bIsVisualizeCollision = false;

    const ImGuiWindowFlags UIWindowFlags =
        ImGuiWindowFlags_NoResize |      // 크기 조절 비활성화
        ImGuiWindowFlags_AlwaysAutoResize;  // 항상 내용에 맞게 크기 조절


    UUIManager::Get()->RegisterUIElement( [this]() {

        if (Character)
        {
            Vector3 CurrentScale = Character->GetWorldTransform().Scale;
            float Restitution = Character->GetRestitution();
            Vector3 CurrentVelo = Character->GetCurrentVelocity();
            Vector3 CurrentPos = Character->GetWorldTransform().Position;
            bool bGravity = Character->IsGravity();
            ImGui::Begin("Charcter", nullptr, UIWindowFlags);
            ImGui::Checkbox("bGravity", &bGravity);
            Character->SetGravity(bGravity);
            ImGui::Text("CurrentVelo : %.2f  %.2f  %.2f", CurrentVelo.x,
                        CurrentVelo.y,
                        CurrentVelo.z);
            FUIVectorDrag::DrawVector3Drag(CurrentPos,
                                             FDragParameters::Position(1.0f,"%.1f"),
                                             "Position",nullptr,
                                             [&](const Vector3& InPos)
                                             {
                                                 Character->SetPosition(InPos);
                                                 return true;
                                             });
            FUIVectorDrag::DrawVector3Drag(CurrentScale,
                                           FDragParameters::Scale(0.1f, "%.1f"),
                                           "Scale", nullptr,
                                           [&](const Vector3& InScale)
                                           {
                                               Character->SetScale(InScale);
                                               return true;
                                           });
            if (ImGui::Button("CompTree")) 
            {
                Character->GetRootComp()->PrintComponentTree();
            }
            if (ImGui::DragFloat("Restitution", &Restitution, 0.1f, 0.0001f, 1.5f, "%.2f"))
            {
                Character->SetRestitution(Restitution);
            }
            ImGui::End();
        }

        if (Character2)
        {
            Vector3 CurrentPos = Character2->GetWorldTransform().Position;
            Quaternion CurrentRot = Character2->GetWorldTransform().Rotation;
            Vector3 CurrentLocalPos = Character2->GetRootComp()->GetLocalTransform().Position;
            Quaternion CurrentLocalRot = Character2->GetRootComp()->GetLocalTransform().Rotation;
            Vector3 CurrentVelo = Character2->GetCurrentVelocity();
            bool bGravity2 = Character2->IsGravity();
            bool bIsActive2 = Character2->IsActive();
            float Restitution = Character2->GetRestitution();
            bool bStatic2 = Character2->GetComponentByType<URigidBodyComponent>()->IsStatic();
            ImGui::Begin("Charcter2", nullptr, UIWindowFlags);
            ImGui::Checkbox("bGravity", &bGravity2);
            if(ImGui::Checkbox("Static", &bStatic2))
            {
                if(bStatic2)
                {
                    Character2->GetComponentByType<URigidBodyComponent>()->SetRigidType(ERigidBodyType::Static);
                }
                else
                {
                    Character2->GetComponentByType<URigidBodyComponent>()->SetRigidType(ERigidBodyType::Dynamic);
                }
                
            }
            Character2->SetGravity(bGravity2);
            ImGui::Text("CurrentVelo : %.2f  %.2f  %.2f", CurrentVelo.x,
                        CurrentVelo.y,
                        CurrentVelo.z);
            FUIVectorDrag::DrawVector3Drag(CurrentPos,
                                           FDragParameters::Position(1.0f, "%.1f"),
                                           "Position", nullptr,
                                           [&](const Vector3& InPos)
                                           {
                                               Character2->SetPosition(InPos);
                                               return true;
                                           });
            FUIVectorDrag::DrawVector3Drag(CurrentLocalPos,
                                           FDragParameters::Position(1.0f, "%.1f"),
                                           "LocalPosition", nullptr,
                                           [&](const Vector3& InPos)
                                           {
                                               Character2->GetRootComp()->SetLocalPosition(InPos);
                                               return true;
                                           });
            ImGui::End();
        }

        if (Character && Character2)
        {
            ImGui::Begin("Both", nullptr, UIWindowFlags);
            ImGui::SetNextItemWidth(50.0f);
            if (ImGui::InputFloat("MaxSpeed", &MaxSpeed, 0.0f, 0.0f, "%.02f"))
            {
                SetMaxSpeeds(MaxSpeed);
            }
            ImGui::SetNextItemWidth(50.0f);
            ImGui::InputFloat("PowerMags", &PowerMagnitude, 0.0f, 0.0f, "%.02f");
            if (ImGui::Checkbox("CollisionVisualize", &bIsVisualizeCollision))
            {
                auto Colli = Character->GetComponentByType<UCollisionComponentBase>();
                if (Colli)
                {
                    Colli->SetDebugVisualize(bIsVisualizeCollision);
                }
                auto Colli2 = Character2->GetComponentByType<UCollisionComponentBase>();
                if (Colli2)
                {
                    Colli2->SetDebugVisualize(bIsVisualizeCollision);
                }
            }
            static bool bLink = false;
            if (ImGui::Checkbox("Link", &bLink))
            {
                if (bLink)
                {
                    //트랜스폼 테스트
					Character->GetRootComp()->AddChild(Character2->GetRootComp());
                }
                else
                {
                    Character2->GetRootComp()->SetParent(nullptr);
                }
            }
            ImGui::End();
        }
        
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


    UInputAction AMoveUp_P2("AMoveUp_P2");
    AMoveUp_P2.KeyCodes = { 'I' };

    UInputAction AMoveDown_P2("AMoveDown_P2");
    AMoveDown_P2.KeyCodes = { 'K' };

    UInputAction AMoveRight_P2("AMoveRight_P2");
    AMoveRight_P2.KeyCodes = { 'L' };

    UInputAction AMoveLeft_P2("AMoveLeft_P2");
    AMoveLeft_P2.KeyCodes = { 'J' };


    UInputAction CameraFollowObject("CameraFollowObject");
    CameraFollowObject.KeyCodes = { 'V' };

    UInputAction CameraLookTo("CameraLookTo");
    CameraLookTo.KeyCodes = { VK_F2 };

    auto WeakCharacter = std::weak_ptr(Character);
    auto WeakCamera = std::weak_ptr(Camera);
    auto WeakCharacter2 = std::weak_ptr(Character2);

    // 캐릭터 1 입력 바인딩
    InputContext->BindAction(AMoveUp_P1,
                             EKeyEvent::Pressed,
                             WeakCharacter,
                             [this](const FKeyEventData& EventData) {
                                 float InForceMagnitude = CharacterMass * PowerMagnitude;

                                 if (!Camera)
                                     return;

                                 if (EventData.bShift)
                                 {
                                     
                                     Character->ApplyForce(1e3 * Camera->GetWorldUp() * InForceMagnitude);
                                     
                                 }
                                 else
                                 {
                                     Character->ApplyForce(1e3 * Camera->GetWorldForward() * InForceMagnitude);
                                 }
                             },
                             "CharacterMove");

    InputContext->BindAction(AMoveDown_P1,
                             EKeyEvent::Pressed,
                             WeakCharacter,
                             [this](const FKeyEventData& EventData) {
                                 float InForceMagnitude = CharacterMass * PowerMagnitude;

                                 if (!Camera)
                                     return;

                                 if (EventData.bShift)
                                 {

                                     Character->ApplyForce(1e3 * -Camera->GetWorldUp() * InForceMagnitude);

                                 }
                                 else
                                 {
                                     Character->ApplyForce(1e3 * -Camera->GetWorldForward() * InForceMagnitude);
                                 }
                             },
                             "CharacterMove");

    InputContext->BindAction(AMoveRight_P1,
                             EKeyEvent::Pressed,
                             WeakCharacter,
                             [this](const FKeyEventData& EventData) {
                                 float InForceMagnitude = CharacterMass * PowerMagnitude;

                                 if (!Camera)
                                     return;
                                 Character->ApplyForce(1e3 * Camera->GetWorldRight() * InForceMagnitude);
                             },
                             "CharacterMove");

    InputContext->BindAction(AMoveLeft_P1,
                             EKeyEvent::Pressed,
                             WeakCharacter,
                             [this](const FKeyEventData& EventData) {
                                 float InForceMagnitude = CharacterMass * PowerMagnitude;

                                 if (!Camera)
                                     return;
                                 Character->ApplyForce(1e3 * -Camera->GetWorldRight() * InForceMagnitude);
                             },
                             "CharacterMove");


      // 캐릭터 2 입력 바인딩
    InputContext->BindAction(AMoveUp_P2,
                             EKeyEvent::Pressed,
                             WeakCharacter2,
                             [this](const FKeyEventData& EventData) {
                                 float InForceMagnitude = Character2Mass * PowerMagnitude;

                                 if (!Camera)
                                     return;

                                 if (EventData.bShift)
                                 {
                                     Character2->ApplyForce(1e3 * Camera->GetWorldUp() * InForceMagnitude);
                                 }
                                 else
                                 {
                                     Character2->ApplyForce(1e3 * Camera->GetWorldForward() * InForceMagnitude);
                                 }
                             },
                             "CharacterMove");

    InputContext->BindAction(AMoveDown_P2,
                             EKeyEvent::Pressed,
                             WeakCharacter2,
                             [this](const FKeyEventData& EventData) {
                                 float InForceMagnitude = Character2Mass * PowerMagnitude;

                                 if (!Camera)
                                     return;

                                 if (EventData.bShift)
                                 {
                                     Character2->ApplyForce(1e3 * -Camera->GetWorldUp() * InForceMagnitude);
                                 }
                                 else
                                 {
                                     Character2->ApplyForce(1e3 * -Camera->GetWorldForward() * InForceMagnitude);
                                 }
                             },
                             "CharacterMove");

    InputContext->BindAction(AMoveRight_P2,
                             EKeyEvent::Pressed,
                             WeakCharacter2,
                             [this](const FKeyEventData& EventData) {
                                 float InForceMagnitude = Character2Mass * PowerMagnitude;

                                 if (!Camera)
                                     return;

								 Character2->ApplyForce(1e3 * Camera->GetWorldRight() * InForceMagnitude);                             
                             },
                             "CharacterMove");

    InputContext->BindAction(AMoveLeft_P2,
                             EKeyEvent::Pressed,
                             WeakCharacter2,
                             [this](const FKeyEventData& EventData) {
                                 float InForceMagnitude = Character2Mass * PowerMagnitude;

                                 if (!Camera)
                                     return;

                                 Character2->ApplyForce(1e3 * -Camera->GetWorldRight() * InForceMagnitude);
                             },
                             "CharacterMove");


    UInputManager::Get()->SystemContext->BindAction(CameraFollowObject,
                                                    EKeyEvent::Pressed,
                                                    WeakCamera,
                                                    [this](const FKeyEventData& EventData) {
                                                        Camera->bLookAtObject = !Camera->bLookAtObject;
                                                    },
                                                    "CameraFollowObject");

    UInputManager::Get()->SystemContext->BindAction(CameraLookTo,
                                                    EKeyEvent::Pressed,
                                                    WeakCamera,
                                                    [this](const FKeyEventData& EventData) {
                                                        Camera->LookTo();
                                                    },
                                                    "CameraLookTo");
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
                                                                           //Position correction
                                                                           Character->SetPosition(NewPosition);

                                                                           const Vector3 CurrentVelo = Character->GetCurrentVelocity();
                                                                           const float Restitution = 1.0f;
                                                                           const float VelocityAlongNormal = Vector3::Dot(CurrentVelo, Normal);

                                                                           // 물체가 표면에서 멀어지는 중이면 충격량 없음
                                                                           if (VelocityAlongNormal >= 0.0f)
                                                                           {
                                                                               return;
                                                                           }

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
                                                                            //Position correction
                                                                            Character2->SetPosition(NewPosition);

                                                                            const Vector3 CurrentVelo = Character2->GetCurrentVelocity();
                                                                            const float Restitution = 1.0f;
                                                                            const float VelocityAlongNormal = Vector3::Dot(CurrentVelo, Normal);

                                                                            // 물체가 표면에서 멀어지는 중이면 충격량 없음
                                                                            if (VelocityAlongNormal >= 0.0f)
                                                                            {
                                                                                return;
                                                                            }

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
