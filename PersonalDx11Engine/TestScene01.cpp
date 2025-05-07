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
#include <windows.h> // Sleep 함수 사용
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


    //EPA
    CuurentIteration = 1;
    Detector = FCollisionDetector();
    FCollisionDetector::FSimplex Simplex;
    if (Detector.GJKCollision(*Box, Box->GetWorldTransform(),
                              *Sphere, Sphere->GetWorldTransform(),
                              Simplex))
    {
        assert(CreateInitialPolytope(Simplex, Detector, Poly));
    }

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

    static int PrevIteration = 0;
    if (Camera)
    {
        Vector3 NewPos = CalculateSphericPosition(Latitude, Longitude);
        Camera->SetPosition(NewPos);
        Camera->LookAt(Vector3::Zero);
        Camera->Tick(DeltaTime);
    }
    if (bDebug01)
    {
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
    }
 

    auto ToVector = [](const XMVECTOR& InVec) { return Vector3(InVec.m128_f32[0], InVec.m128_f32[1], InVec.m128_f32[2]); };

    if (CuurentIteration > PrevIteration)
    {
        EPACollision(*Box, Box->GetWorldTransform(),
                     *Sphere, Sphere->GetWorldTransform(),
                     Poly, Detector);
    }
    else
    {
        DrawPolytope(Poly, Vector4(1, 1, 1, 1), DeltaTime, true);
    }
    
    PrevIteration = CuurentIteration;
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
        ImGui::Checkbox("CollisionShape", &bDebug01);
        ImGui::Checkbox("PolytopeEPA", &bDebug02);
        ImGui::TextColored(ImVec4(1, 0, 1, 1), "EPAIteration %d", CuurentIteration);
        ImGui::SameLine();
        if (ImGui::Button("+"))
        {
            CuurentIteration++;
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


///////////////

bool UTestScene01::CreateInitialPolytope(const FCollisionDetector::FSimplex& InSimplex,
                                         FCollisionDetector& InDetector,
                                         FCollisionDetector::PolytopeSOA& OutPoly)
{
	bool Result = false;

	using PolytopeSOA = FCollisionDetector::PolytopeSOA;
	using FSimplex = FCollisionDetector::FSimplex;

	constexpr int MaxEPAIterations = 30;

	// 입력 심플렉스가 4면체인지 확인
	if (InSimplex.Size != 4) {
		// 4면체가 아닌 경우 종료
		return Result;
	}

	// EPA에 필요한 보조 데이터 구조
	struct ContactInfo {
		std::vector<XMVECTOR> VerticesA;    // ShapeA의 대응점
		std::vector<XMVECTOR> VerticesB;    // ShapeB의 대응점
	};

	// 다면체 초기화
	PolytopeSOA Poly;
	ContactInfo ContactData;

	// 정점 초기화 (GJK에서 얻은 Simplex로 시작)
	for (int i = 0; i < InSimplex.Size; ++i) {
		Poly.Vertices.push_back(InSimplex.Points[i]);
		ContactData.VerticesA.push_back(InSimplex.SupportPointsA[i]);
		ContactData.VerticesB.push_back(InSimplex.SupportPointsB[i]);
	}

	// 초기 다면체 구성 (4면체)
	static const int faceIndices[][3] = {
		{0, 1, 2}, {0, 3, 1}, {0, 2, 3}, {1, 3, 2}
	};

	// 인덱스와 법선 초기화
	for (int i = 0; i < 4; ++i) {
		// 면의 인덱스 추가
		Poly.Indices.push_back(faceIndices[i][0]);
		Poly.Indices.push_back(faceIndices[i][1]);
		Poly.Indices.push_back(faceIndices[i][2]);

		// 면의 법선 계산
		XMVECTOR v1 = XMVectorSubtract(Poly.Vertices[faceIndices[i][1]], Poly.Vertices[faceIndices[i][0]]);
		XMVECTOR v2 = XMVectorSubtract(Poly.Vertices[faceIndices[i][2]], Poly.Vertices[faceIndices[i][0]]);
		XMVECTOR normal = XMVector3Cross(v1, v2);

		// 법선이 원점에서 멀어지도록 보장
		XMVECTOR toOrigin = XMVectorNegate(Poly.Vertices[faceIndices[i][0]]);
		if (XMVectorGetX(XMVector3Dot(normal, toOrigin)) > 0) {
			normal = XMVectorNegate(normal);
			// 인덱스 순서 변경
			int lastIdx = static_cast<int>(Poly.Indices.size());
			std::swap(Poly.Indices[lastIdx - 2], Poly.Indices[lastIdx - 1]);
		}

		normal = XMVector3Normalize(normal);
		Poly.Normals.push_back(normal);

		// 원점에서 면까지의 거리 계산
		float distance = std::fabs(XMVectorGetX(XMVector3Dot(normal, Poly.Vertices[faceIndices[i][0]])));
		Poly.Distances.push_back(distance);
	}

	OutPoly = Poly;
	return true;

}

bool UTestScene01::EPACollision(const ICollisionShape& ShapeA,
								const FTransform& TransformA,
								const ICollisionShape& ShapeB,
								const FTransform& TransformB, 
								FCollisionDetector::PolytopeSOA& Poly,
								FCollisionDetector& InDetector)
{
	bool Result = false;

	using PolytopeSOA = FCollisionDetector::PolytopeSOA;
	using FSimplex = FCollisionDetector::FSimplex;


	// EPA 반복 수행
	static bool bConverged = false;
    static int Iterations = 0;
	XMVECTOR ClosestNormal = XMVectorZero();
	float ClosestDistance = FLT_MAX;
	int ClosestFaceIndex = -1;
    float IterPauseTime = 1.0f;

    // EPA에 필요한 보조 데이터 구조
    struct ContactInfo {
        std::vector<XMVECTOR> VerticesA;    // ShapeA의 대응점
        std::vector<XMVECTOR> VerticesB;    // ShapeB의 대응점
    } ContactData;

    // 원점에서 가장 가까운 면 찾기
    ClosestFaceIndex = -1;
    ClosestDistance = FLT_MAX;

    for (size_t i = 0; i < Poly.Distances.size(); ++i) {
        if (Poly.Distances[i] < ClosestDistance && Poly.Distances[i] > 0) {
            ClosestDistance = Poly.Distances[i];
            ClosestFaceIndex = static_cast<int>(i);
            ClosestNormal = Poly.Normals[i];
        }
    }

    if (ClosestFaceIndex == -1) {
        // 적합한 면이 없음 (드문 경우)
        LOG_FUNC_CALL("Error : There is No Face");
        return false;
    }

   
	// 가장 가까운 면의 법선 방향으로 새 지원점 찾기
	XMVECTOR SearchDir = ClosestNormal;
	XMVECTOR SupportA, SupportB;
	XMVECTOR NewPoint = InDetector.ComputeMinkowskiSupport(
		ShapeA, TransformA, ShapeB, TransformB, SearchDir, SupportA, SupportB);

	// 새 지원점과 가장 가까운 면 사이의 거리 계산
	float NewDistance = std::abs(XMVectorGetX(XMVector3Dot(NewPoint, SearchDir)));

	// 수렴 확인 (더 이상 진행이 없거나 충분히 가까움)
	if (fabs(NewDistance - ClosestDistance) < KINDA_SMALLER) {
		// 수렴 - 결과 설정
		Vector3 Normal;
		XMStoreFloat3(&Normal, ClosestNormal);
		Normal *= ClosestDistance;

		// 충돌 지점 계산 (법선 방향의 중간점)
		int faceStartIdx = ClosestFaceIndex * 3;
		XMVECTOR ContactPointA = XMVectorZero();
		XMVECTOR ContactPointB = XMVectorZero();

		for (int i = 0; i < 3; ++i) {
			int vertexIndex = Poly.Indices[faceStartIdx + i];
			ContactPointA = XMVectorAdd(ContactPointA, ContactData.VerticesA[vertexIndex]);
			ContactPointB = XMVectorAdd(ContactPointB, ContactData.VerticesB[vertexIndex]);
		}

		ContactPointA = XMVectorScale(ContactPointA, 1.0f / 3.0f);
		ContactPointB = XMVectorScale(ContactPointB, 1.0f / 3.0f);

        //// 반발 방향으로 살짝 이동된 지점 사용
        //XMVECTOR ContactPoint = XMVectorAdd(
        //	ContactPointA,
        //	XMVectorScale(ClosestNormal, ClosestDistance * 0.5f)
        //);

        LOG_FUNC_CALL("EPA Converged");
        Vector3 ContactPoint;
        XMStoreFloat3(&ContactPoint, ContactPointA);
        bConverged = true;
        UDebugDrawManager::Get()->DrawLine(ContactPoint, ContactPoint + Normal, Vector4(0, 1, 0, 1), 0.001f, 0.1f);
        return true;
    }

    // 새 정점 추가
    int NewPointIndex = static_cast<int>(Poly.Vertices.size());
    Poly.Vertices.push_back(NewPoint);
    ContactData.VerticesA.push_back(SupportA);
    ContactData.VerticesB.push_back(SupportB);

    // QuickHull 알고리즘으로 다면체 재구성
    InDetector.UpdatePolytopeWithQuickHull(Poly, NewPointIndex);

	return true;
}


void UTestScene01::DrawPolytope(const FCollisionDetector::PolytopeSOA& Polytope, const Vector4& Color, float LifeTime = 0.1f, bool bDrawNormals = false)
{
    if (!bDebug02)
        return;
    using PolytopeSOA = FCollisionDetector::PolytopeSOA;

    if (Polytope.Indices.empty() || Polytope.Vertices.empty())
        return;

    auto* DebugDrawer = UDebugDrawManager::Get();
    if (!DebugDrawer)
        return;

    // 각 면(triangle)마다 처리
    for (size_t i = 0; i < Polytope.Indices.size(); i += 3)
    {
        if (i + 2 >= Polytope.Indices.size())
            break; // 안전 검사

        // 삼각형의 세 꼭지점 인덱스
        int IdxA = Polytope.Indices[i];
        int IdxB = Polytope.Indices[i + 1];
        int IdxC = Polytope.Indices[i + 2];

        // 인덱스 유효성 검사
        if (IdxA >= Polytope.Vertices.size() || IdxB >= Polytope.Vertices.size() || IdxC >= Polytope.Vertices.size() ||
            IdxA < 0 || IdxB < 0 || IdxC < 0)
            continue;

        // 삼각형 꼭지점 좌표
        Vector3 VertA, VertB, VertC;
        XMStoreFloat3(&VertA, Polytope.Vertices[IdxA]);
        XMStoreFloat3(&VertB, Polytope.Vertices[IdxB]);
        XMStoreFloat3(&VertC, Polytope.Vertices[IdxC]);

        // 삼각형 외곽선 그리기
        DebugDrawer->DrawLine(VertA, VertB, Color, 0.001f,LifeTime);
        DebugDrawer->DrawLine(VertB, VertC, Color, 0.001f,LifeTime);
        DebugDrawer->DrawLine(VertC, VertA, Color, 0.001f,LifeTime);
    }

    // 추가적으로 면의 법선 시각화 (선택적)
    if (bDrawNormals && Polytope.Normals.size() * 3 >= Polytope.Indices.size())
    {
        Vector4 InvalidNormalColor = Vector4(1.0f, 0.0f, 0.0f, 1.0f); // Red
        Vector4 ValidNormalColor = Vector4(0.0f, 0.0f, 1.0f, 1.0f); // Blue
        float NormalLength = 0.2f; // 법선 길이

        for (size_t i = 0; i < Polytope.Normals.size(); i++)
        {
            // 삼각형의 중심점 계산
            size_t TriIdx = i * 3;
            if (TriIdx + 2 >= Polytope.Indices.size())
                break;

            int IdxA = Polytope.Indices[TriIdx];
            int IdxB = Polytope.Indices[TriIdx + 1];
            int IdxC = Polytope.Indices[TriIdx + 2];

            if (IdxA >= Polytope.Vertices.size() || IdxB >= Polytope.Vertices.size() || IdxC >= Polytope.Vertices.size() ||
                IdxA < 0 || IdxB < 0 || IdxC < 0)
                continue;

            // 삼각형 중심 계산
            XMVECTOR TriCenter = XMVectorScale(
                XMVectorAdd(XMVectorAdd(Polytope.Vertices[IdxA], Polytope.Vertices[IdxB]), Polytope.Vertices[IdxC]),
                1.0f / 3.0f);

            //법선 원점 방향 검사
            Vector4 NormalColor = ValidNormalColor;
            // 법선이 원점을 향하면 INvlaid
            XMVECTOR toOrigin = XMVectorNegate(Polytope.Vertices[IdxA]);
            if (XMVectorGetX(XMVector3Dot(Polytope.Normals[i], toOrigin)) > KINDA_SMALL) {
                NormalColor = InvalidNormalColor;
            }

            // 법선 벡터 계산
            XMVECTOR NormalEnd = XMVectorAdd(TriCenter, XMVectorScale(Polytope.Normals[i], NormalLength));

            // 법선 그리기
            Vector3 Start, End;
            XMStoreFloat3(&Start, TriCenter);
            XMStoreFloat3(&End, NormalEnd);
            DebugDrawer->DrawLine(Start, End, NormalColor, 0.001f,LifeTime);
        }
    }
}