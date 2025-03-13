﻿#include <windows.h>
#include <iostream>

//ImGui
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_impl_dx11.h"
#include "imGui/imgui_impl_win32.h"

#include "Debug.h"
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

//Contents
#include "Random.h"
#include "ElasticBody.h"

//test
//#include "testDynamicAABBTree.h"

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

constexpr int CONSOLE_WIDTH = 500;
constexpr int CONSOLE_HEIGHT = 300;

using namespace std;

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

void CreateConsole(int consoleWidth, int consoleHeight, int xPos, int yPos)
{
	// 콘솔 할당
	AllocConsole();

	// 표준 입출력 스트림을 콘솔로 리다이렉션
	FILE* pConsole;
	freopen_s(&pConsole, "CONOUT$", "w", stdout);
	freopen_s(&pConsole, "CONIN$", "r", stdin);

	// 콘솔 창의 핸들 가져오기
	HWND consoleWindow = GetConsoleWindow();

	// 콘솔 창의 크기와 위치 설정
	RECT rect;
	GetWindowRect(consoleWindow, &rect);
	MoveWindow(consoleWindow, xPos, yPos, consoleWidth, consoleHeight, TRUE);

	// 콘솔 버퍼 크기 설정
	HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(consoleHandle, &csbi);

	COORD bufferSize;
	bufferSize.X = static_cast<SHORT>(min(consoleWidth / 8, SHRT_MAX)); // 대략적인 문자 너비로 나눔
	//bufferSize.Y = static_cast<SHORT>(min(consoleHeight / 16, SHRT_MAX)); // 대략적인 문자 높이로 나눔
	bufferSize.Y = static_cast<SHORT>(min(consoleHeight, SHRT_MAX)); 
	SetConsoleScreenBufferSize(consoleHandle, bufferSize);

	// 콘솔 창 스타일 설정 (필요시)
	// SetWindowLong(consoleWindow, GWL_STYLE, GetWindowLong(consoleWindow, GWL_STYLE) & ~WS_MAXIMIZEBOX & ~WS_SIZEBOX);
}

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

	WCHAR WindowClass[] = L"MyPersonal11Engine";
	WCHAR Title[] = L"Scene1";

	// 각종 메시지를 처리할 함수인 WndProc의 함수 포인터를 WindowClass 구조체에 넣는다.
	WNDCLASSW wndclass = { 0, WindowProc, 0, 0, 0, 0, 0, 0, 0, WindowClass };
	RegisterClassW(&wndclass);

	//윈도우 생성
	HWND hWnd = CreateWindowExW(0, WindowClass, Title, WS_POPUP | WS_VISIBLE | WS_OVERLAPPEDWINDOW,
								CW_USEDEFAULT, CW_USEDEFAULT, SCREEN_WIDTH, SCREEN_HEIGHT,
								nullptr, nullptr, hInstance, nullptr);
#pragma endregion

	RECT appRect;
	GetWindowRect(hWnd, &appRect); //윈도우 크기 가져오기

	// 콘솔 생성
	CreateConsole(CONSOLE_WIDTH, CONSOLE_HEIGHT, appRect.right , appRect.bottom - CONSOLE_HEIGHT);

	//Renderer
	auto Renderer = make_unique<URenderer>();
	Renderer->Initialize(hWnd);
	Renderer->SetVSync(true);

	//ModelBufferManager Init
	UModelBufferManager::Get()->SetDevice(Renderer->GetDevice());
	assert(UModelBufferManager::Get()->Initialize());

	//LoadTexture
	auto TAbstract = make_shared<ID3D11ShaderResourceView*>();
	assert(LoadTextureFromFile(Renderer->GetDevice(), TEXTURE01, TAbstract.get()), "Texture Load Failed");

	auto TPole = make_shared<ID3D11ShaderResourceView*>();
	assert(LoadTextureFromFile(Renderer->GetDevice(), TEXTURE02, TPole.get()), "Texture Load Failed");
	
	auto TTile = make_shared<ID3D11ShaderResourceView*>();
	assert(LoadTextureFromFile(Renderer->GetDevice(), TEXTURE03, TTile.get()), "Texture Load Failed");

	auto TRock = make_shared<ID3D11ShaderResourceView*>();
	assert(LoadTextureFromFile(Renderer->GetDevice(), TEXTURE04, TRock.get()), "Texture Load Failed");
	
	ID3D11ShaderResourceView* TDefault = nullptr;
	
	//Shader
	auto Shader = make_unique<UShader>();
	D3D11_INPUT_ELEMENT_DESC textureShaderLayout[] =
	{
		//SemanticName, SemanticIndex, Foramt, InputSlot, AlignByteOffset, 
		// InputSlotClass,InstanceDataStepRate
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	ID3D11SamplerState* SamplerState = Renderer->GetDefaultSamplerState();

	Shader->Initialize(Renderer->GetDevice(), MYSHADER, MYSHADER, textureShaderLayout, ARRAYSIZE(textureShaderLayout));
	Shader->Bind(Renderer->GetDeviceContext(), SamplerState);
	//Shader->BindTexture(Renderer->GetDeviceContext(), *TDefault.get(), ETextureSlot::Albedo); //defaultTexure

	// 여기에서 ImGui를 생성합니다.
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	ImGui_ImplWin32_Init((void*)hWnd);
	ImGui_ImplDX11_Init(Renderer->GetDevice(), Renderer->GetDeviceContext());
	//ImGui set
	ImGui::SetNextWindowPos(ImVec2(2.0f, 2.0f), ImGuiCond_FirstUseEver);//영구위치설정
	ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiCond_FirstUseEver); //자동조절

	bool bIsExit = false;

	//delta frame time
	LARGE_INTEGER frequency;
	LARGE_INTEGER lastTime;
	LARGE_INTEGER currentTime;

	QueryPerformanceFrequency(&frequency);
	QueryPerformanceCounter(&lastTime);

	auto CubeModel = UModelBufferManager::Get()->GetCubeModel();
	auto SphereModel = UModelBufferManager::Get()->GetSphereModel();

#pragma region Object Initialization
	auto Camera = UCamera::Create(PI / 4.0f, SCREEN_WIDTH , SCREEN_HEIGHT, 0.1f, 100.0f);
	Camera->SetPosition({ 0,0.0f,-10.0f });

	float CharacterMass = 5.0f;
	float Character2Mass = 15.0f;
	
	auto Character = UGameObject::Create<UGameObject>(CubeModel);
	Character->SetScale(0.25f * Vector3::One);
	Character->SetPosition({ 0,0,0 });
	Character->bDebug = true;

	auto RigidComp1 = UActorComponent::Create<URigidBodyComponent>();
	RigidComp1->SetMass(CharacterMass);

	auto CollisionComp1 = UActorComponent::Create<UCollisionComponent>();
	CollisionComp1->SetShapeBox();
	CollisionComp1->SetHalfExtent(0.5f * Character->GetTransform()->GetScale());
	CollisionComp1->BindRigidBody(RigidComp1);
	CollisionComp1->SetActive(false);

	Character->AddActorComponent(RigidComp1);

	auto Character2 = UGameObject::Create<UElasticBody>();
	Character2->SetScale(0.75f * Vector3::One);
	Character2->SetPosition({ 1,0,0 });
	Character2->SetShapeSphere();
	Character2->bDebug = true;
	
#pragma endregion
	Camera->PostInitialized();
	Character->PostInitialized();
	Character2->PostInitialized();

	Camera->SetLookAtObject(Character.get());
	Camera->LookTo(Character->GetTransform()->GetPosition());
	Camera->bLookAtObject = false;

	Character->PostInitializedComponents();
	Character2->PostInitializedComponents();
	Camera->PostInitializedComponents();

	//Border
	const float XBorder = 3.0f;
	const float YBorder = 2.0f;
	const float ZBorder = 5.0f;
#pragma region Border Restitution Trigger(temporary)
	auto IsInBorder = [XBorder, YBorder, ZBorder](const Vector3& Position)
		{
			return  std::abs(Position.x) < XBorder &&
				std::abs(Position.y) < YBorder &&
				std::abs(Position.z) < ZBorder;
		};

	Character->GetTransform()->OnTransformChangedDelegate.Bind(Character,
															  [&IsInBorder, &Character, XBorder, YBorder, ZBorder](const FTransform& InTransform)
															  {
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
															   [&IsInBorder, &Character2, XBorder, YBorder, ZBorder](const FTransform& InTransform)
															   {
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

#pragma endregion
#pragma region InputAction
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

#pragma endregion
#pragma region  Input Action Bind

	auto InputContext01 = UInputContext::Create("Scene01");
	UInputManager::Get()->RegisterInputContext(InputContext01);

	//Character
	InputContext01->BindAction(AMoveUp_P1,
							   EKeyEvent::Pressed,
							   Character,
							   [&Character, &CharacterMass](const FKeyEventData& EventData) {
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

	InputContext01->BindAction(AMoveDown_P1,
							   EKeyEvent::Pressed,
							   Character,
							   [&Character, &CharacterMass](const FKeyEventData& EventData) {
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

	InputContext01->BindAction(AMoveRight_P1,
							   EKeyEvent::Pressed,
							   Character,
							   [&Character, &CharacterMass](const FKeyEventData& EventData) {
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

	InputContext01->BindAction(AMoveLeft_P1,
							   EKeyEvent::Pressed,
							   Character,
							   [&Character, &CharacterMass](const FKeyEventData& EventData) {
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

	InputContext01->BindAction(AMoveStop_P1,
							   EKeyEvent::Pressed,
							   Character,
							   [&Character](const FKeyEventData& EventData) {
								   Character->StopMove();
							   },
							   "CharacterMove");

	InputContext01->BindAction(AMoveUp_P2,
							   EKeyEvent::Pressed,
							   Character2,
							   [&Character2, &Character2Mass](const FKeyEventData& EventData) {
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

	InputContext01->BindAction(AMoveDown_P2,
							   EKeyEvent::Pressed,
							   Character2,
							   [&Character2, &Character2Mass](const FKeyEventData& EventData) {
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

	InputContext01->BindAction(AMoveRight_P2,
							   EKeyEvent::Pressed,
							   Character2,
							   [&Character2, &Character2Mass](const FKeyEventData& EventData) {
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

	InputContext01->BindAction(AMoveLeft_P2,
							   EKeyEvent::Pressed,
							   Character2,
							   [&Character2, &Character2Mass](const FKeyEventData& EventData) {
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

	InputContext01->BindAction(AMoveStop_P2,
							   EKeyEvent::Pressed,
							   Character2,
							   [&Character2](const FKeyEventData& EventData) {
								   Character2->StopMove();
							   },
							   "CharacterMove");

	UInputManager::Get()->SystemContext->BindAction(CameraUp,
													EKeyEvent::Pressed,
													Camera,
													[&Camera](const FKeyEventData& EventData) {
														Camera->StartMove(Vector3::Forward);
													},
													"CameraMove");

	UInputManager::Get()->SystemContext->BindAction(CameraDown,
													EKeyEvent::Pressed,
													Camera,
													[&Camera](const FKeyEventData& EventData) {
														Camera->StartMove(-Vector3::Forward);
													},
													"CameraMove");

	UInputManager::Get()->SystemContext->BindAction(CameraRight,
													EKeyEvent::Pressed,
													Camera,
													[&Camera](const FKeyEventData& EventData) {
														Camera->StartMove(Vector3::Right);
													},
													"CameraMove");

	UInputManager::Get()->SystemContext->BindAction(CameraLeft,
													EKeyEvent::Pressed,
													Camera,
													[&Camera](const FKeyEventData& EventData) {
														Camera->StartMove(-Vector3::Right);
													},
													"CameraMove");

	UInputManager::Get()->SystemContext->BindAction(CameraFollowObject,
													EKeyEvent::Pressed,
													Camera,
													[&Camera](const FKeyEventData& EventData) {
														Camera->bLookAtObject = !Camera->bLookAtObject;
													},
													"CameraMove");

	UInputManager::Get()->SystemContext->BindAction(CameraLookTo,
													EKeyEvent::Pressed,
													Camera,
													[&Camera](const FKeyEventData& EventData) {
														Camera->LookTo();
													},
													"CameraMove");

	UInputManager::Get()->SystemContext->BindActionSystem(Debug1,
													EKeyEvent::Pressed,
													[&Character2](const FKeyEventData& EventData) {
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
#pragma endregion
	
	Character->SetActive(false);
	Character2->SetActive(false);

	float accumTime = 0.0f;
	const float SPAWNFREQ = 0.75f;
	bool bSpawnBody = true;
	vector<UElasticBody*> tmpVecs;

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
			// 키 입력 메시지를 번역
			TranslateMessage(&msg);
			// 메시지를 적절한 윈도우 프로시저에 전달
			DispatchMessage(&msg);

			if (msg.message == WM_QUIT)
			{
				bIsExit = true;
				break;
			}
		}
#pragma endregion

#pragma region logic
		//Draw Debug
		FDebugDrawManager::Get()->Tick(DeltaTime);

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

		accumTime += DeltaTime;
		if(accumTime > SPAWNFREQ)
		{
			accumTime = 0.0f;
			if (bSpawnBody)
			{
				auto tmpBody = UGameObject::Create<UElasticBody>();
				tmpBody->SetScale(FRandom::RandF(0.5f, 0.8f)* Vector3::One);
				tmpBody->SetPosition(FRandom::RandVector(Vector3::One * -1.5f, Vector3::One * 1.5f));
				tmpBody->SetMass(FRandom::RandF(1.0f, 5.0f));
				tmpBody->SetShapeSphere();
				tmpBody->bDebug = true;
				tmpBody->SetDebugColor((Vector4)FRandom::RandVector({ 0,0,0 }, { 1,1,1 }));
				tmpBody->PostInitialized();
				tmpBody->PostInitializedComponents();
				tmpBody->SetActive(true);
				tmpVecs.push_back(tmpBody.get());
				//UCollisionManager::Get()->PrintTreeStructure();
				LOG("ElasticBody Count : %03d", tmpVecs.size());
			}
		}
		
		for (auto tmp : tmpVecs)
		{
			tmp->Tick(DeltaTime);
		}
		//UElasticBodyManager::Get()->Tick(DeltaTime);
		UCollisionManager::Get()->Tick(DeltaTime);
	

#pragma endregion 
		
#pragma region Rendering
		//before render
		Renderer->BeforeRender();

		//Render
		Renderer->RenderGameObject(Camera.get(),Character.get(), Shader.get(), *TTile.get());
		Renderer->RenderGameObject(Camera.get(),Character2.get(), Shader.get(), *TPole.get());
		for (auto tmp : tmpVecs)
		{
			Renderer->RenderGameObject(Camera.get(), tmp, Shader.get(), *TPole.get());
		}

		//UElasticBodyManager::Get()->Render(Renderer.get(), Camera.get(), Shader.get(), *TTile.get());
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


	
		FDebugDrawManager::Get()->DrawAll(Camera.get());

		//RenderUI
		ImGui::Render();
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
#pragma endregion
		//end render
		Renderer->EndRender();
#pragma endregion

	}//end main Loop
#pragma endregion 

	// 여기에서 ImGui 소멸
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	Renderer->Release();

#pragma region COM
	CoUninitialize();
#pragma endregion
	return 0;
}

