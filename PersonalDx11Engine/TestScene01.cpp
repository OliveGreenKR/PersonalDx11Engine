#include "TestScene01.h"
#include "ConfigReadManager.h"
#include "Camera.h"
#include "InputManager.h"
#include "InputContext.h"
#include "RenderDataTexture.h"
#include "Renderer.h"
#include "UIManager.h"
#include "DebugDrawerManager.h"
#include "define.h"

#include  "BoxComponent.h"
#include "SphereComponent.h"

#include "CollisionDetector.h"

UTestScene01::UTestScene01()
{
    InputContext = UInputContext::Create(SceneName);
}

UTestScene01::~UTestScene01()
{
    if (ObjectPool)
    {
        ObjectPool->ClearAllActives();
        ObjectPool.reset();
    }
    if (InputContext)
    {
        InputContext = nullptr;
    }
}

void UTestScene01::Initialize()
{
    int VIEW_WIDTH, VIEW_HEIGHT;
    UConfigReadManager::Get()->GetValue("ScreenWidth", VIEW_WIDTH);
    UConfigReadManager::Get()->GetValue("ScreenHeight", VIEW_HEIGHT);

    // 카메라 설정
    Camera = UCamera::Create(PI / 4.0f, VIEW_WIDTH, VIEW_HEIGHT, 0.1f, 100.0f);
    Camera->SetPosition({ 0, 0.0f, -CameraDistance });
    Camera->PostInitialized();
    Camera->PostInitializedComponents();

    Latitude = 0.0f;
    Longitude = 0.0f;

    Box = UActorComponent::Create<UBoxComponent>();
    Sphere = UActorComponent::Create<USphereComponent>();

    const  Vector3 Pos1 = Vector3(-0.35f, 0, 0);
    const  Vector3 Pos2 = Vector3(0.35f, 0, 0);
    const  Vector3 Scale1 = Vector3::One;
    const  Vector3 Scale2 = Vector3::One * 0.75f;

    Box->SetWorldPosition(Pos1);
    Box->SetWorldScale(Scale1);

    Sphere->SetWorldPosition(Pos2);
    Sphere->SetWorldScale(Scale2);


    //원점 표시
    auto OPointWeak = ObjectPool->AcquireForcely();
    auto OPoint = OPointWeak.Get();
    OPoint->SetColor(Vector4(1, 0, 1, 1));
    OPoint->SetModel(FResourceHandle(MDL_SPHERE_Low));
    OPoint->SetWorldScale(Vector3::One * 0.1f);

    SetupInput();
    
}

void UTestScene01::Load()
{
    // 입력 컨텍스트 등록
    UInputManager::Get()->RegisterInputContext(InputContext);

    ObjectPool = std::make_unique<TFixedObjectPool<UPrimitiveComponent, 256>>();
}

void UTestScene01::Unload()
{
    UInputManager::Get()->UnregisterInputContext(SceneName);

    if (ObjectPool)
    {
        ObjectPool.reset();
    }

    Camera = nullptr;
    Box = nullptr;
    Sphere = nullptr;
}

void UTestScene01::Tick(float DeltaTime)
{
    if (Camera)
    {
        Vector3 NewPos = CalculateSphericPosition(Latitude, Longitude);
        Camera->SetPosition(NewPos);
        Camera->LookAt(Vector3::Zero);
        Camera->Tick(DeltaTime);
    }

    auto TransformA = Box->GetWorldTransform();
    auto TransformB = Sphere->GetWorldTransform();

    UDebugDrawManager::Get()->DrawBox(
        TransformA.Position,
        TransformA.Scale,
        TransformA.Rotation,
        Vector4(1, 1, 0, 1),
        DeltaTime
    );

    UDebugDrawManager::Get()->DrawSphere(
        TransformB.Position,
        TransformB.Scale.x * 0.5f,
        TransformB.Rotation,
        Vector4(0, 1, 1, 1),
        DeltaTime
    );

    static FCollisionDetector Detector;

    auto ToVector = [](const XMVECTOR& InVec) { return Vector3(InVec.m128_f32[0], InVec.m128_f32[1], InVec.m128_f32[2]); };

    FCollisionDetector::FSimplex Simplex;
    if (Detector.GJKCollision(*Box, Box->GetWorldTransform(),
                              *Sphere, Sphere->GetWorldTransform(),
                              Simplex))
    {
        Vector3 A = ToVector(Simplex.Points[0]);
        Vector3 B = ToVector(Simplex.Points[1]);
        Vector3 C = ToVector(Simplex.Points[2]);
        Vector3 D = ToVector(Simplex.Points[3]);
        Vector4 Color = Vector4(1, 1, 1, 1);

        float thickness = KINDA_SMALL;
        //A
        UDebugDrawManager::Get()->DrawLine(A, B, Color, thickness, DeltaTime);
        UDebugDrawManager::Get()->DrawLine(A, C, Color, thickness, DeltaTime);
        UDebugDrawManager::Get()->DrawLine(A, D, Color, thickness, DeltaTime);
        //Bc
        UDebugDrawManager::Get()->DrawLine(B, C, Color, thickness, DeltaTime);
        //cd
        UDebugDrawManager::Get()->DrawLine(C, D, Color, thickness, DeltaTime);
        //db
        UDebugDrawManager::Get()->DrawLine(D, B, Color, thickness, DeltaTime);
    }


}

void UTestScene01::SubmitRender(URenderer* Renderer)
{
    for (auto body : *ObjectPool)
    {
        FRenderJob RenderJob = Renderer->AllocateRenderJob<FRenderDataTexture>();
        auto Primitive = body.Get();
        if (Primitive)
        {
            if (Primitive->FillRenderData(GetMainCamera(), RenderJob.RenderData))
            {
                Renderer->SubmitJob(RenderJob);
            }
        }
    }


}

void UTestScene01::SubmitRenderUI()
{
    const ImGuiWindowFlags UIWindowFlags =
        ImGuiWindowFlags_NoResize |      // 크기 조절 비활성화
        ImGuiWindowFlags_AlwaysAutoResize;  // 항상 내용에 맞게 크기 조절

    static int BodyNum = 0;
    UUIManager::Get()->RegisterUIElement("TestSceneUI", [this]() {
        ImGui::Begin("TestScene");
        if (ImGui::InputFloat("Latidue", &Latitude, 1.0f, 10.0f))
        {
            Latitude = Math::Clamp(Latitude, -LatitudeThreshold, LatitudeThreshold);
        }
        if (ImGui::InputFloat("Longitud", &Longitude, 1.0f, 10.0f))
        {
            Latitude = Math::Clamp(Latitude, -LongitudeThreshold, LongitudeThreshold);
        }
        ImGui::End();
                                         });
}

void UTestScene01::HandleInput(const FKeyEventData& EventData)
{
    return;
}

UCamera* UTestScene01::GetMainCamera() const
{
	return Camera.get();
}

std::string& UTestScene01::GetName()
{
    return SceneName;
}

void UTestScene01::SetupInput()
{
    UInputAction CameraUp("CameraUp");
    CameraUp.KeyCodes = { VK_UP };

    UInputAction CameraDown("CameraDown");
    CameraDown.KeyCodes = { VK_DOWN };

    UInputAction CameraRight("CameraRight");
    CameraRight.KeyCodes = { VK_RIGHT };

    UInputAction CameraLeft("CameraLeft");
    CameraLeft.KeyCodes = { VK_LEFT };

	UInputManager::Get()->SystemContext->BindActionSystem(CameraUp,
														  EKeyEvent::Repeat,
														  [this](const FKeyEventData& EventData) {
															  Latitude += 5;
															  Latitude = Math::Clamp(Latitude, -LatitudeThreshold, LatitudeThreshold);
														  },
														  "CameraMove");

	UInputManager::Get()->SystemContext->BindActionSystem(CameraDown,
														  EKeyEvent::Repeat,
														  [this](const FKeyEventData& EventData) {
															  Latitude -= 5;
															  Latitude = Math::Clamp(Latitude, -LatitudeThreshold, LatitudeThreshold);
														  },
														  "CameraMove");

	UInputManager::Get()->SystemContext->BindActionSystem(CameraRight,
														  EKeyEvent::Repeat,
														  [this](const FKeyEventData& EventData) {
															  Longitude += 2.5;
															  Longitude = Math::Clamp(Longitude, -LongitudeThreshold, LongitudeThreshold);
														  },
														  "CameraMove");

	UInputManager::Get()->SystemContext->BindActionSystem(CameraLeft,
														  EKeyEvent::Repeat,
														  [this](const FKeyEventData& EventData) {
															  Longitude -= 2.5;
															  Longitude = Math::Clamp(Longitude, -LongitudeThreshold, LongitudeThreshold);
														  },
														  "CameraMove");
}

Vector3 UTestScene01::CalculateSphericPosition(float Latitude, float Longitude)
{
    float radius = CameraDistance;

    // 위도와 경도를 라디안으로 변환 (입력이 도(degree) 단위라고 가정)
    float latRad = Math::DegreeToRad(Latitude);      // 위도
    float lonRad = Math::DegreeToRad(Longitude);     // 경도

    // x, y, z 좌표 계산
    float x = radius  * cos(latRad) * sin(lonRad);   // x 좌표
    float y = radius  * sin(latRad);                 // y 좌표 (위도에 따라 높이)
    float z = -radius * cos(latRad) * cos(lonRad);   // z 좌표

    return Vector3(x, y, z);
}
