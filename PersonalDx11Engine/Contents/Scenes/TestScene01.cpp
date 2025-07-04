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
#include "Color.h"
#include "Random.h"

using namespace DirectX;

UTestScene01::UTestScene01()
{
    InputContext = UInputContext::Create(SceneName.c_str());
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
    Camera = UCamera::Create(PI / 4.0f, VIEW_WIDTH, VIEW_HEIGHT, 0.1f, 5000.0f);
    Camera->SetPosition({ 0, 0.0f, -800.0f });
    Camera->PostInitialized();
    Camera->PostInitializedComponents();

    bEPAConverged = false;

    //멤버 초기화
    Box = UActorComponent::Create<UBoxComponent>();
    Sphere = UActorComponent::Create<USphereComponent>();
    Detector = FCollisionDetector();

    //초기 테스트환경
    RandSpawn(); 

    //원점 표시
    auto OPointWeak = ObjectPool->AcquireForcely();
    auto OPoint = OPointWeak.Get();
    OPoint->BroadcastPostInitialized();
    OPoint->BroadcastPostTreeInitialized();
    OPoint->SetColor(Vector4(1, 0, 1, 1));
    OPoint->SetModel(FResourceHandle(MDL_SPHERE_Low));
    OPoint->SetWorldScale(Vector3::One() * 100.0f * 0.05f);

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
    UInputManager::Get()->UnregisterInputContext(SceneName.c_str());

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
    if (bEPAConverged)
    {
        CuurentIteration = PrevIteration;
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
            Primitive->BroadcastPostInitialized();
            Primitive->BroadcastPostTreeInitialized();
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
    UUIManager::Get()->RegisterUIElement([this]() {
        ImGui::Begin("TestScene");
        ImGui::Checkbox("CollisionShape", &bDebug01);
        ImGui::Checkbox("PolytopeEPA", &bDebug02);
        ImGui::TextColored(ImVec4(1, 0, 1, 1), "EPAIteration %d", CuurentIteration);
        ImGui::SameLine();
        if (ImGui::Button("+"))
        {
            CuurentIteration++;
        }
        if (ImGui::Button("NewCase"))
        {
            RandSpawn();
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

void UTestScene01::RandSpawn()
{
    if (!Box || !Sphere)
        return;

    Poly = FCollisionDetector::PolytopeSOA();

    Vector3 Pos1 = FRandom::RandVector(-50.0f * Vector3::One(), 50.0f * Vector3::One());
    Vector3 Pos2 = FRandom::RandVector(-50.0f * Vector3::One(), 50.0f * Vector3::One());

    Vector3 Scale1 = Vector3::One() * 100.0f;
    Vector3 Scale2 = Vector3::One() * 75.0f;

    Quaternion Quat1 = FRandom::RandQuat();
    Quaternion Quat2 = FRandom::RandQuat();

    FTransform NewTransform1;
    NewTransform1.Position = Pos1;
    NewTransform1.Scale = Scale1;
    NewTransform1.Rotation = Quat1;

    Box->SetWorldTransform(NewTransform1);

    FTransform NewTransform2;
    NewTransform2.Position = Pos2;
    NewTransform2.Scale = Scale2;
    NewTransform2.Rotation = Quat2;

    Sphere->SetWorldTransform(NewTransform2);

    //EPA Init
    CuurentIteration = 1;
    FCollisionDetector::FSimplex Simplex;
    if (Detector.GJKCollision(*Box, Box->GetWorldTransform(),
                              *Sphere, Sphere->GetWorldTransform(),
                              Simplex))
    {
        if(!CreateInitialPolytope(Simplex, Detector, Poly))
        {
            for (auto& point : Simplex.Points)
            {
                Vector3 pos;
                XMStoreFloat3(&pos, point);
                UDebugDrawManager::Get()->DrawSphere(pos, 5.0f, Quaternion::Identity(),
                                                     Color::Red, 3.0f, false);
            }
        }
    }
    else
    {
        LOG("TestScene01 : Failed to Detect Collison in GJK");
        for (auto& point : Simplex.Points)
        {
            Vector3 pos;
            XMStoreFloat3(&pos, point);
            UDebugDrawManager::Get()->DrawSphere(pos, 5.0f, Quaternion::Identity(),
                                                 Color::Red, 3.0f, false);
        }
        
    }
}

void UTestScene01::SetupInput()
{
}
///////////////

bool UTestScene01::CreateInitialPolytope(const FCollisionDetector::FSimplex& InSimplex,
                                         FCollisionDetector& InDetector,
                                         FCollisionDetector::PolytopeSOA& OutPoly)
{
	bool Result = false;

	using PolytopeSOA = FCollisionDetector::PolytopeSOA;
	using FSimplex = FCollisionDetector::FSimplex;

    static int MaxEPAIterations = 20;

    // EPA requires a starting simplex that encloses the origin (a tetrahedron)
    if (InSimplex.Size != 4) {
        LOG_FUNC_CALL("Error: Initial simplex size is not 4 (%d). EPA requires a tetrahedron.", InSimplex.Size);
        Result = false; // Cannot perform EPA without a volume
        return Result;
    }

    // Initialize the polytope (convex hull) and corresponding support points
    PolytopeSOA Poly;

    // Initialize vertices from the GJK simplex
    // Reserve space to avoid reallocations during expansion
    Poly.Vertices.reserve(MaxEPAIterations + 4);
    Poly.VerticesA.reserve(MaxEPAIterations + 4);
    Poly.VerticesB.reserve(MaxEPAIterations + 4);

    for (int i = 0; i < InSimplex.Size; ++i) {
        Poly.Vertices.push_back(InSimplex.Points[i]);
        Poly.VerticesA.push_back(InSimplex.SupportPointsA[i]);
        Poly.VerticesB.push_back(InSimplex.SupportPointsB[i]);
    }

    // Define indices for the initial tetrahedron faces
    // Order is important for initial normal calculation
    static const int faceIndices[][3] = {
        {0, 1, 2}, {0, 3, 1}, {0, 2, 3}, {1, 3, 2}
    };

    // Initialize faces (indices, normals, distances) for the tetrahedron
    Poly.Indices.reserve(4 * 3); // 4 faces, 3 indices each
    Poly.Normals.reserve(4);
    Poly.Distances.reserve(4);

    for (int i = 0; i < 4; ++i) {
        // Add face indices
        Poly.Indices.push_back(faceIndices[i][0]);
        Poly.Indices.push_back(faceIndices[i][1]);
        Poly.Indices.push_back(faceIndices[i][2]);

        // Get vertex vectors using SIMD
        XMVECTOR v0 = Poly.Vertices[faceIndices[i][0]];
        XMVECTOR v1 = Poly.Vertices[faceIndices[i][1]];
        XMVECTOR v2 = Poly.Vertices[faceIndices[i][2]];

        // Calculate edge vectors using SIMD
        XMVECTOR edge1 = XMVectorSubtract(v1, v0);
        XMVECTOR edge2 = XMVectorSubtract(v2, v0);

        // Calculate potential face normal using SIMD cross product
        XMVECTOR normal = XMVector3Cross(edge1, edge2);

        // Ensure the normal points away from the origin
        // Check the dot product with a vector from a vertex to the origin (-v0).
        // If the dot product is positive, the normal points towards the origin, so flip it.
        if (XMVectorGetX(XMVector3Dot(normal, XMVectorNegate(v0))) > KINDA_SMALL) { // Use tolerance for robustness
            normal = XMVectorNegate(normal);
            // Reverse winding order if normal is flipped to maintain consistency
            size_t lastIdx = Poly.Indices.size();
            std::swap(Poly.Indices[lastIdx - 2], Poly.Indices[lastIdx - 1]);
        }
        else if (XMVectorGetX(XMVector3Dot(normal, XMVectorNegate(v0))) < -KINDA_SMALL) {
            // Normal is pointing away, which is good.
        }
        else {
            // Normal is close to perpendicular to the vector to origin (origin near face plane)
            // Let's assume valid initial simplex and check magnitude.
            if (XMVectorGetX(XMVector3LengthSq(normal)) < KINDA_SMALL * KINDA_SMALL) {
                LOG_FUNC_CALL("Error: Degenerate face found during initial polytope setup.");
                // Remove the face indices if degenerate? Or just proceed hoping EPA fixes it.
                Result = false; // Cannot perform EPA without a volume
                return Result;
            }
        }

        // Normalize the normal using SIMD
        normal = XMVector3Normalize(normal);
        Poly.Normals.push_back(normal);

        // Calculate distance from origin to the face along the normal direction.
        // Since normal points away, this dot product should be non-negative.
        float distance = XMVectorGetX(XMVector3Dot(normal, v0));
        Poly.Distances.push_back(distance);
    }


    OutPoly = std::move(Poly);
    return true;

}

bool UTestScene01::EPACollision(const ICollisionShape& ShapeA,
                                const FTransform& TransformA,
                                const ICollisionShape& ShapeB,
                                const FTransform& TransformB,
                                FCollisionDetector::PolytopeSOA& Poly,
                                FCollisionDetector& InDetector)
{
    using PolytopeSOA = FCollisionDetector::PolytopeSOA;
    using FSimplex = FCollisionDetector::FSimplex;

    int ClosestFaceIndex = -1;
    XMVECTOR ClosestNormal = XMVectorZero();
    float ClosestDistance = FLT_MAX;


	int num_faces = Poly.Distances.size(); // Use current size as faces are added/removed
	for (int i = 0; i < num_faces; ++i) {
		// Find the minimum distance among faces whose normal points away from origin (distance >= 0)
		if (Poly.Distances[i] < ClosestDistance && Poly.Distances[i] >= -KINDA_SMALL) { // Check >= -tolerance for robustness
			ClosestDistance = Poly.Distances[i];
			ClosestFaceIndex = i;
		}
	}

	// If no closest face found (shouldn't happen with a valid polytope)
	if (ClosestFaceIndex == -1) {
		LOG_FUNC_CALL("Error: No closest face found in EPA loop. Polytope may be invalid.");
        return false;
	}

	// Get the normal of the closest face
	ClosestNormal = Poly.Normals[ClosestFaceIndex];

	// Search direction is the normal of the closest face
	XMVECTOR SearchDir = ClosestNormal;

	// Find a new support point in the search direction (farthest point on Minkowski difference)
	XMVECTOR SupportA, SupportB;
	XMVECTOR NewPoint = Detector.ComputeMinkowskiSupport(
		ShapeA, TransformA, ShapeB, TransformB, SearchDir, SupportA, SupportB);

    Vector3 SpA;
    XMStoreFloat3(&SpA, SupportA);
    Vector3 SpB;
    XMStoreFloat3(&SpB, SupportB);
    Vector3 SDir;
    XMStoreFloat3(&SDir, SearchDir);
    SDir *= 100.0f;
    float DrawDuration = 2.0f;
    UDebugDrawManager::Get()->DrawSphere(SpA, 5.0f, Quaternion::Identity(), Color::ColorHex(0xd964b4), DrawDuration, false);
    UDebugDrawManager::Get()->DrawSphere(SpB, 5.0f, Quaternion::Identity(), Color::ColorHex(0x45dc3c), DrawDuration, false);
    UDebugDrawManager::Get()->DrawLine(SpA, SpA + SDir, Vector4(1, 0, 0, 1), 1.0f, DrawDuration, false);

	// Calculate the distance of the new point from the origin along the search direction
	float NewPointDistance = std::fabs(XMVectorGetX(XMVector3Dot(NewPoint, SearchDir)));

	// Check for convergence
	// If the new point is not significantly further along the normal than the closest face's distance,
	// the polytope has expanded sufficiently. The origin is effectively on the closest face plane.
	if (std::fabs(NewPointDistance - ClosestDistance) < KINDA_SMALL) {
		// Converged - the current closest face represents the penetration data
        Vector3 Normal;
        DirectX::XMStoreFloat3(&Normal, ClosestNormal);
        Normal *= ClosestDistance;
        UDebugDrawManager::Get()->DrawLine(Vector3::Zero(), Normal, Vector4(1, 0, 0, 1), 0.001f, 1.0f);


		// Calculate contact point
		// A common approximation is the midpoint between the average of support points on A
		// and the average of support points on B for the vertices of the closest face.
		int faceStartIdx = ClosestFaceIndex * 3;
		XMVECTOR ContactPointA_Sum = XMVectorZero();
		XMVECTOR ContactPointB_Sum = XMVectorZero();

		for (int i = 0; i < 3; ++i) {
			int vertexIndex = Poly.Indices[faceStartIdx + i];
			// Accumulate corresponding support points from ContactData
			ContactPointA_Sum = XMVectorAdd(ContactPointA_Sum, Poly.VerticesA[vertexIndex]);
			ContactPointB_Sum = XMVectorAdd(ContactPointB_Sum, Poly.VerticesB[vertexIndex]);
		}

		// Average the sums
		XMVECTOR ContactPointA_Avg = XMVectorScale(ContactPointA_Sum, 1.0f / 3.0f);
		XMVECTOR ContactPointB_Avg = XMVectorScale(ContactPointB_Sum, 1.0f / 3.0f);

		// Approximate the contact point as the midpoint between the averaged support points
		XMVECTOR ResultContactPoint = XMVectorScale(XMVectorAdd(ContactPointA_Avg, ContactPointB_Avg), 0.5f);
        Vector3 Point;
		XMStoreFloat3(&Point, ResultContactPoint);
        LOG("EPA Result Point : %s", Debug::ToString(Point));
        bEPAConverged = true;
		// Successfully found penetration data
		return true; // Exit EPA function early
	}

	// If not converged, add the new point to the polytope vertices
	int NewPointIndex = static_cast<int>(Poly.Vertices.size());
	Poly.Vertices.push_back(NewPoint);
	Poly.VerticesA.push_back(SupportA); // Store corresponding support point 
	Poly.VerticesB.push_back(SupportB);

	// Expand the polytope by removing faces visible from the new point
	// and creating new faces connecting the new point to the horizon edges.
	Detector.UpdatePolytope(Poly, NewPointIndex);
    return false;
}


void UTestScene01::DrawPolytope(const FCollisionDetector::PolytopeSOA& Polytope, const Vector4& Color, float LifeTime = 0.1f, bool bDrawNormals = false)
{
    if (!bDebug02)
        return;
    Detector.DrawPolytope(Polytope, LifeTime, bDrawNormals, Color);
}