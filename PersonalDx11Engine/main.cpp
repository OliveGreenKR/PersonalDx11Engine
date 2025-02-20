#include <windows.h>

//ImGui
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_impl_dx11.h"
#include "imGui/imgui_impl_win32.h"

#include "Utils.h"
#include "DebugDrawManager.h"

#include <memory>
#include "define.h"
#include "Renderer.h"
#include "Math.h"
#include "D3D.h"
#include "Model.h"
#include "D3DShader.h"
#include "RscUtil.h"
#include "GameObject.h"
#include "Camera.h"
#include "InputManager.h"
#include "RigidBodyComponent.h"
#include "Color.h"

#include "CollisionComponent.h"
#include "CollisionManager.h"
//#include "CollisionDetector.h"
//#include "CollisionResponseCalculator.h"
//#include "CollisionEventDispatcher.h"


#define KEY_UP 'W'
#define KEY_DOWN 'S'
#define KEY_LEFT 'A'
#define KEY_RIGHT 'D'

#define KEY_UP2 'I'
#define KEY_DOWN2 'K'
#define KEY_LEFT2 'J'
#define KEY_RIGHT2 'L'

constexpr int SCREEN_WIDTH = 800;
constexpr int SCREEN_HEIGHT = 800;

using namespace std;

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

//struct for Processing Win Msgs
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
	{
		return true;
	}

	if (UInputManager::Get()->ProcessWindowsMessage(message, wParam, lParam))
	{
		return 0;
	}

	switch (message)
	{
		case WM_DESTROY:
			// Signal that the app should quit
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}


//Main
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
#pragma region COM
	HRESULT hr = CoInitialize(nullptr);
	if (FAILED(hr))
		return false;
#pragma endregion
#pragma region window init

	WCHAR WindowClass[] = L"JungleWindowClass";
	WCHAR Title[] = L"Game Tech Lab";

	// ���� �޽����� ó���� �Լ��� WndProc�� �Լ� �����͸� WindowClass ����ü�� �ִ´�.
	WNDCLASSW wndclass = { 0, WindowProc, 0, 0, 0, 0, 0, 0, 0, WindowClass };
	RegisterClassW(&wndclass);

	//������ ����
	HWND hWnd = CreateWindowExW(0, WindowClass, Title, WS_POPUP | WS_VISIBLE | WS_OVERLAPPEDWINDOW,
								CW_USEDEFAULT, CW_USEDEFAULT, SCREEN_WIDTH, SCREEN_HEIGHT,
								nullptr, nullptr, hInstance, nullptr);
#pragma endregion

	//Renderer
	auto Renderer = make_unique<URenderer>();
	Renderer->Initialize(hWnd);
	Renderer->SetVSync(true);

	//LoadTexture
	auto TAbstract = make_shared<ID3D11ShaderResourceView*>();
	assert(LoadTextureFromFile(Renderer->GetDevice(), TEXTURE01, TAbstract.get()), "Texture Load Failed");

	auto TPole = make_shared<ID3D11ShaderResourceView*>();
	assert(LoadTextureFromFile(Renderer->GetDevice(), TEXTURE02, TPole.get()), "Texture Load Failed");
	
	auto TTile = make_shared<ID3D11ShaderResourceView*>();
	assert(LoadTextureFromFile(Renderer->GetDevice(), TEXTURE03, TTile.get()), "Texture Load Failed");

	auto TRock = make_shared<ID3D11ShaderResourceView*>();
	assert(LoadTextureFromFile(Renderer->GetDevice(), TEXTURE04, TRock.get()), "Texture Load Failed");
	
	auto TDefault = TAbstract;
	
	//Shader
	auto Shader = make_unique<UShader>();
	D3D11_INPUT_ELEMENT_DESC textureShaderLayout[] =
	{
		//SemanticName, SemanticIndex, Foramt, InputSlot, AlignByteOffset, 
		// InputSlotClass,InstanceDataStepRate
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	auto DebugShader = make_unique<UShader>();

	ID3D11SamplerState* SamplerState = Renderer->GetDefaultSamplerState();

	Shader->Initialize(Renderer->GetDevice(), MYSHADER, MYSHADER, textureShaderLayout, ARRAYSIZE(textureShaderLayout));
	Shader->Bind(Renderer->GetDeviceContext(), SamplerState);
	Shader->BindTexture(Renderer->GetDeviceContext(), *TDefault.get(), ETextureSlot::Albedo); //defaultTexure

	// ���⿡�� ImGui�� �����մϴ�.
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	ImGui_ImplWin32_Init((void*)hWnd);
	ImGui_ImplDX11_Init(Renderer->GetDevice(), Renderer->GetDeviceContext());
	//ImGui set
	ImGui::SetNextWindowPos(ImVec2(2.0f, 2.0f), ImGuiCond_FirstUseEver);//������ġ����
	ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiCond_FirstUseEver); //�ڵ�����

	const ImGuiWindowFlags UIWindowFlags =
		ImGuiWindowFlags_NoResize |      // ũ�� ���� ��Ȱ��ȭ
		ImGuiWindowFlags_AlwaysAutoResize;  // �׻� ���뿡 �°� ũ�� ����


	bool bIsExit = false;

	//delta frame time
	LARGE_INTEGER frequency;
	LARGE_INTEGER lastTime;
	LARGE_INTEGER currentTime;

	QueryPerformanceFrequency(&frequency);
	QueryPerformanceCounter(&lastTime);

	//Model Data
	auto TriModel = UModel::GetDefaultTriangle(Renderer->GetDevice());
	auto CubeModel = UModel::GetDefaultCube(Renderer->GetDevice());
	auto SphereModel = UModel::GetDefaultSphere(Renderer->GetDevice());

#pragma region Object Initialization
	auto Camera = UCamera::Create(PI / 4.0f, SCREEN_WIDTH / SCREEN_HEIGHT, 0.1f, 100.0f);
	//Camera->SetPosition({ 0,0.0f,-12.0f });
	Camera->SetPosition({ 0,3.0f,-7.0f });

	auto Floor = UGameObject::Create(CubeModel);
	Floor->SetScale({ 5.0f,0.1f,5.0f });
	Floor->SetPosition({ 0,-1.0f,0 });
	
	auto Character = UGameObject::Create(CubeModel);
	Character->SetScale(0.25f * Vector3::One);
	Character->SetPosition({ 0,0,0 });
	Character->bDebug = true;
	
	auto Character2 = UGameObject::Create(SphereModel);
	Character2->SetScale(0.75f * Vector3::One);
	Character2->SetPosition({ 1.0f,0,0 });
	Character2->bDebug = true;
#pragma endregion
	Camera->PostInitialized();
	Floor->PostInitialized();
	Character->PostInitialized();
	Character2->PostInitialized();

	Camera->SetLookAtObject(Character);
	Camera->LookTo(Character->GetTransform()->GetPosition());
	Camera->bLookAtObject = false;
#pragma region Actor Components Initialization
	//rigid
	auto RigidComp1 = UActorComponent::Create<URigidBodyComponent>();
	auto RigidComp2 = UActorComponent::Create<URigidBodyComponent>();
	auto RigidComp3 = UActorComponent::Create<URigidBodyComponent>();
	RigidComp3->SetRigidType(ERigidBodyType::Static);

	//rigid attach
	Character->AddActorComponent(RigidComp1);
	Character2->AddActorComponent(RigidComp2);
	Floor->AddActorComponent(RigidComp3);

	//Collision
	auto CollisionComp1 = UActorComponent::Create<UCollisionComponent>(ECollisionShapeType::Box, 0.5f * Character->GetTransform()->GetScale());
	auto CollisionComp2 = UActorComponent::Create<UCollisionComponent>(ECollisionShapeType::Sphere, 0.5f * Character2->GetTransform()->GetScale());
	auto CollisionComp3 = UActorComponent::Create<UCollisionComponent>(ECollisionShapeType::Box, 0.5f * Floor->GetTransform()->GetScale());

	CollisionComp2->OnCollisionEnter.BindSystem([](const FCollisionEventData& InColliision)
												{
													FDebugDrawManager::Get().DrawArrow(
														InColliision.CollisionDetectResult.Point,
														InColliision.CollisionDetectResult.Normal,
														0.5f,
														2.0f,
														Vector4(1,0,1,0.5f),
														0.3f
														);
												}, "OnCollisionBegin_Sys_Comp2");
	
	//Collision Attach
	CollisionComp1->BindRigidBody(RigidComp1);
	CollisionComp2->BindRigidBody(RigidComp2);
	CollisionComp3->BindRigidBody(RigidComp3);
#pragma endregion
	Character->PostInitializedComponents();
	Character2->PostInitializedComponents();
	Camera->PostInitializedComponents();

	//�ݸ��� ������Ʈ ���
	UCollisionManager::Get()->RegisterCollision(CollisionComp1);
	UCollisionManager::Get()->RegisterCollision(CollisionComp2);
	UCollisionManager::Get()->RegisterCollision(CollisionComp3);

#pragma region  InputBind
	//input Action Bind - TODO::  Abstactionize 'Input Action'
	//����� ��ü�� ���� ������ ������ Ű �̺�Ʈ�� ����..
	constexpr WPARAM ACTION_MOVE_UP_P1 = 'W';
	constexpr WPARAM ACTION_MOVE_DOWN_P1 = 'S';
	constexpr WPARAM ACTION_MOVE_RIGHT_P1 = 'D';
	constexpr WPARAM ACTION_MOVE_LEFT_P1 = 'A';
	constexpr WPARAM ACTION_MOVE_STOP_P1 = 'F';

	constexpr WPARAM ACTION_MOVE_UP_P2 = 'I';
	constexpr WPARAM ACTION_MOVE_DOWN_P2 = 'K';
	constexpr WPARAM ACTION_MOVE_RIGHT_P2 = 'L';
	constexpr WPARAM ACTION_MOVE_LEFT_P2 = 'J';
	constexpr WPARAM ACTION_MOVE_STOP_P2 = 'H';

	constexpr WPARAM ACTION_CAMERA_UP = VK_UP;
	constexpr WPARAM ACTION_CAMERA_DOWN = VK_DOWN;
	constexpr WPARAM ACTION_CAMERA_RIGHT = VK_RIGHT;
	constexpr WPARAM ACTION_CAMERA_LEFT = VK_LEFT;
	constexpr WPARAM ACTION_CAMERA_FOLLOWOBJECT = 'V';
	constexpr WPARAM ACTION_CAMERA_LOOKTO = VK_F2;

	constexpr WPARAM ACTION_DEBUG_1 = VK_F1;

	//Character
	UInputManager::Get()->BindKeyEvent(
		EKeyEvent::Pressed,
		Character,
		[&Character](const FKeyEventData& EventData) {
			switch (EventData.KeyCode)
			{
				case(ACTION_MOVE_UP_P1) :
				{
					if (EventData.bShift)
					{
						Character->StartMove(Vector3::Forward);
					}
					else
					{
						Character->ApplyForce(Vector3::Forward * 100.0f);
					}
					break;
				}
				case(ACTION_MOVE_DOWN_P1):
				{
					if (EventData.bShift)
					{
						Character->StartMove(-Vector3::Forward);
					}
					else
					{
						Character->ApplyForce(-Vector3::Forward * 100.0f);
					}
					break;
				}
				case(ACTION_MOVE_RIGHT_P1):
				{
					if (EventData.bShift)
					{
						Character->StartMove(Vector3::Right);
					}
					else
					{
						Character->ApplyForce(Vector3::Right * 100.0f);
					}
					break;
				}
				case(ACTION_MOVE_LEFT_P1):
				{
					if (EventData.bShift)
					{
						Character->StartMove(-Vector3::Right);
					}
					else
					{
						Character->ApplyForce(-Vector3::Right * 100.0f);
					}
					break;
				}
				case(ACTION_MOVE_STOP_P1):
				{
					Character->StopMove();
				}
			}
		},
		"CharacterMove");

	//Character2
	UInputManager::Get()->BindKeyEvent(
		EKeyEvent::Pressed,
		Character2,
		[&Character2](const FKeyEventData& EventData) {
			switch (EventData.KeyCode)
			{
				case(ACTION_MOVE_UP_P2):
				{
					if (EventData.bShift)
					{
						Character2->StartMove(Vector3::Forward);
					}
					else
					{
						Character2->ApplyForce(Vector3::Forward * 100.0f);
					}
					break;
				}
				case(ACTION_MOVE_DOWN_P2):
				{
					if (EventData.bShift)
					{
						Character2->StartMove(-Vector3::Forward);
					}
					else
					{
						Character2->ApplyForce(-Vector3::Forward * 100.0f);
					}
					break;
				}
				case(ACTION_MOVE_RIGHT_P2):
				{
					if (EventData.bShift)
					{
						Character2->StartMove(Vector3::Right);
					}
					else
					{
						Character2->ApplyForce(Vector3::Right * 100.0f);
					}
					break;
				}
				case(ACTION_MOVE_LEFT_P2):
				{
					if (EventData.bShift)
					{
						Character2->StartMove(-Vector3::Right);
					}
					else
					{
						Character2->ApplyForce(-Vector3::Right * 100.0f);
					}
					break;
				}
				case(ACTION_MOVE_STOP_P2):
				{
					Character2->StopMove();
				}
			}
		},
		"CharacterMove");
	//Camera
	UInputManager::Get()->BindKeyEvent(
		EKeyEvent::Pressed,
		Camera,
		[&Camera](const FKeyEventData& EventData) {
			switch (EventData.KeyCode)
			{
				case(ACTION_CAMERA_UP):
				{
					Camera->StartMove(Vector3::Forward);
					break;
				}
				case(ACTION_CAMERA_DOWN):
				{
					Camera->StartMove(-Vector3::Forward);
					break;
				}
				case(ACTION_CAMERA_RIGHT):
				{
					Camera->StartMove(Vector3::Right);
					break;
				}
				case(ACTION_CAMERA_LEFT):
				{
					Camera->StartMove(-Vector3::Right);
					break;
				}
				case(ACTION_CAMERA_FOLLOWOBJECT):
				{
					Camera->bLookAtObject = !Camera->bLookAtObject;
					break;
				}
				case(ACTION_CAMERA_LOOKTO):
				{
					Camera->LookTo();
					break;
				}
			}
		},
		"CharacterMove");
	//Safe Delegate test
	UInputManager::Get()->BindKeyEventSystem(
		EKeyEvent::Pressed,
		[&Character2](const FKeyEventData& EventData) {
			if (EventData.KeyCode == ACTION_DEBUG_1)
			{
				//ȸ�� ���� �׽�Ʈ
				if (Character2.get())
				{
					Vector3 TargetPos = Character2->GetTransform()->GetPosition();

					TargetPos += Vector3::Right * 0.15f;
					TargetPos += Vector3::Up * 0.15f;
					Character2->GetRootActorComp()->FindChildByType<URigidBodyComponent>()->ApplyImpulse(
						Vector3::Right *1.0f,
						TargetPos);
				}
			}
		},
		"DEBUG1");



#pragma endregion

#pragma region MainLoop
	while (bIsExit == false)
	{
		MSG msg;

		//record deltaTime
		QueryPerformanceCounter(&currentTime);
		float DeltaTime = static_cast<float>(currentTime.QuadPart - lastTime.QuadPart) / frequency.QuadPart;
		lastTime = currentTime;

#pragma region WinMsgProc
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			// Ű �Է� �޽����� ����
			TranslateMessage(&msg);
			// �޽����� ������ ������ ���ν����� ����
			DispatchMessage(&msg);

			if (msg.message == WM_QUIT)
			{
				bIsExit = true;
				break;
			}
		}
#pragma endregion

#pragma region logic

		FDebugDrawManager::Get().Tick(DeltaTime);

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

		//Draw Debug
		UCollisionManager::Get()->Tick(DeltaTime);

#pragma endregion 
		
#pragma region Rendering
		//before render
		Renderer->BeforeRender();

		//Render
		Renderer->RenderGameObject(Camera.get(),Character.get(), Shader.get(), *TTile.get());
		Renderer->RenderGameObject(Camera.get(),Character2.get(), Shader.get(), *TPole.get());
		Renderer->RenderGameObject(Camera.get(),Floor.get(), Shader.get(), *TRock.get());

#pragma region UI
		// ImGui UI 
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
		ImGui::SetNextWindowSize(ImVec2(150, 20));
		ImGui::SetNextWindowPos(ImVec2(SCREEN_WIDTH - 150,0));
		ImGui::Begin("##DebugText", nullptr,
					 ImGuiWindowFlags_NoTitleBar |
					 ImGuiWindowFlags_NoResize |
					 ImGuiWindowFlags_NoMove |
					 ImGuiWindowFlags_NoInputs |
					 ImGuiWindowFlags_NoBackground |
					 ImGuiWindowFlags_AlwaysAutoResize);
		ImGui::Text("FPS : %.2f", 1.0f/DeltaTime);
		ImGui::End();


		if (Camera)
		{
			ImGui::Begin("Camera", nullptr, UIWindowFlags);
			ImGui::Checkbox("bIs2D", &Camera->bIs2D);
			ImGui::Checkbox("bLookAtObject", &Camera->bLookAtObject);
			ImGui::Text(Utils::ToString(*Camera->GetTransform()));
			ImGui::End();
		}
		
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
			ImGui::Text("Position : %.2f  %.2f  %.2f", Character->GetTransform()->GetPosition().x,
						Character->GetTransform()->GetPosition().y,
						Character->GetTransform()->GetPosition().z);
			ImGui::Text("Rotation : %.2f  %.2f  %.2f", Character->GetTransform()->GetEulerRotation().x,
						Character->GetTransform()->GetEulerRotation().y,
						Character->GetTransform()->GetEulerRotation().z);
			ImGui::Text(Utils::ToString(CollisionComp1->GetPreviousTransform()));
			ImGui::End();
		}
		
		if (Character2)
		{
			Vector3 CurrentVelo = Character2->GetCurrentVelocity();
			bool bGravity2 = Character2->IsGravity();
			bool bPhysics2 = Character2->IsPhysicsSimulated();
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


		FDebugDrawManager::Get().DrawAll(Camera.get());

		ImGui::Render();
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
#pragma endregion

		//end render
		Renderer->EndRender();
#pragma endregion

	}//end main Loop
#pragma endregion 

	// ���⿡�� ImGui �Ҹ�
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	Renderer->Release();

#pragma region COM
	CoUninitialize();
#pragma endregion
	return 0;
}

