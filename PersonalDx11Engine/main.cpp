#include <windows.h>
#include <iostream>

//ImGui
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_impl_dx11.h"
#include "imGui/imgui_impl_win32.h"

#include "ConfigReadManager.h"
#include "Debug.h"
#include <memory>
#include "Renderer.h"
#include "Math.h"
#include "D3D.h"
#include "D3DShader.h"
#include "VertexShader.h"
#include "PixelShader.h"

#include "InputManager.h"
#include "TypeCast.h"


#include "define.h"
#include "PhysicsSystem.h"
#include "CollisionProcessor.h"

#include "SceneManager.h"
#include "GameplayScene01.h"
#include "GameplayScene02.h"
#include "TestScene01.h"

#include "ResourceManager.h"
#include "UIManager.h"

#include "FramePoolManager.h"
#include "DebugDrawerManager.h"

//test
#include "testDynamicAABBTree.h"
#include "testSceneComponent.h"
#include "testPhysicsStateArray.h"

#include "CameraOrbitControl.h"

//System Configs
int SCREEN_WIDTH = 800;
int SCREEN_HEIGHT = 800;
int CONSOLE_WIDTH = 500;
int CONSOLE_HEIGHT = 300;
bool VSYNC = false;
int INIT_SCENE_INDEX = 0;

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
}

//struct for Processing Win Msgs
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam);

	if (UInputManager::Get()->ProcessWindowsMessage(message, wParam, lParam))
	{
		return true;
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

void LoadSystemConfig()
{
	UConfigReadManager::Get()->GetValue("ScreenWidth", SCREEN_WIDTH);
	UConfigReadManager::Get()->GetValue("ScreenHeight", SCREEN_HEIGHT);
	UConfigReadManager::Get()->GetValue("ConsoleWidth", CONSOLE_WIDTH);
	UConfigReadManager::Get()->GetValue("ConsoleHeight", CONSOLE_HEIGHT);
	UConfigReadManager::Get()->GetValue("bVSync", VSYNC);
	UConfigReadManager::Get()->GetValue("InitSceneIndex", INIT_SCENE_INDEX);
}


//Main
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
#pragma region COM
	HRESULT hr = CoInitialize(nullptr);
	if (FAILED(hr))
		return false;
#pragma endregion

	//Load System Configs
	LoadSystemConfig();

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

#pragma region temp Testing code Execution
	//TestDynamicAABBTree::RunAllTestsIterate(std::cout, 100, 500, 500, 500);
	//std::string input;
	//std::getline(std::cin, input); // 사용자 입력을 기다림
	//return 0;

	//TestSceneComponent::RunTransformTest(std::cout, 20, 3);
	//std::string input;
	//std::getline(std::cin, input); // 사용자 입력을 기다림
	//return 0;

	FTestPhysicsState TestPhysicsState;
	std::string input;
	auto PrintResult = [](bool InBool) {
		LOG("%s", InBool ? "PASS" : "FAIL"); };

	//PrintResult ( TestPhysicsState.TestBasicLifecycle() );
	//std::getline(std::cin, input); 

	//PrintResult ( TestPhysicsState.TestIDReuseSystem() );
	//std::getline(std::cin, input); 
	//
	//PrintResult ( TestPhysicsState.TestCompactionSystem() );
	//std::getline(std::cin, input); 

	//PrintResult ( TestPhysicsState.TestActivationManagement() );
	//std::getline(std::cin, input); 

	//PrintResult ( TestPhysicsState.TestStatusQueries() );
	//std::getline(std::cin, input); 

	//PrintResult ( TestPhysicsState.TestResizing() );
	//std::getline(std::cin, input); 

	PrintResult ( TestPhysicsState.TestIntegratedScenarios() );
	std::getline(std::cin, input); 
	return 0;
#pragma endregion

	//Hardware
	auto RenderHardware = make_shared<FD3D>();
	assert(RenderHardware->Initialize(hWnd));
	RenderHardware->SetVSync(VSYNC);

	//Renderer
	auto rhPtr = Engine::Cast<IRenderHardware>(RenderHardware);
	auto Renderer = make_unique<URenderer>();
	Renderer->Initialize(hWnd, rhPtr);

	//ResourceManager
	UResourceManager::Get()->Initialize(RenderHardware.get());
	
	//UIManager
	UUIManager::Get()->Initialize(hWnd, RenderHardware->GetDevice(), RenderHardware->GetDeviceContext());

	bool bIsExit = false;

	//delta frame time
	LARGE_INTEGER frequency;
	LARGE_INTEGER lastTime;
	LARGE_INTEGER currentTime;

	QueryPerformanceFrequency(&frequency);
	QueryPerformanceCounter(&lastTime);

	//Default Shader for renderng
	auto VSShaderHandle = UResourceManager::Get()->LoadResource<UVertexShader>(VS_DEFAULT);
	auto PSShaderHandle = UResourceManager::Get()->LoadResource<UPixelShader>(PS_DEFAULT);
	auto VShader = VSShaderHandle.Get<UVertexShader>();
	auto PShader = PSShaderHandle.Get<UPixelShader>();
	if (VShader && PShader)
	{
		Renderer->GetRenderContext()->BindShader(VShader->GetShader(), PShader->GetShader(), VShader->GetInputLayout());
	}

	//Scene
	auto GameplayScene01 = make_shared<UGameplayScene01>();
	USceneManager::Get()->RegisterScene(GameplayScene01);

	auto GameplayScene02 = make_shared<UGameplayScene02>();
	USceneManager::Get()->RegisterScene(GameplayScene02);

	auto TestScene01 = make_shared<UTestScene01>();
	USceneManager::Get()->RegisterScene(TestScene01);

	//Defualt Scene Load
	if (!USceneManager::Get()->ChangeScene(INIT_SCENE_INDEX))
	{
		USceneManager::Get()->ChangeScene(GameplayScene01->GetName());
	}

#pragma region Global Camera Orbit Controll
	//global Camera Control
	auto CameraOrbitController = std::make_unique<FCameraOrbit>();
	
	//Register InputContext
	auto SystemContext = UInputManager::Get()->SystemContext;

	constexpr float stepRad = 180.0f * PI / 180.0f;

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
														  [&](const FKeyEventData& EventData) {
															  UCamera* Camera = USceneManager::Get()->GetActiveCamera();
															  CameraOrbitController->UpdateOrbit(Camera, 
																								 0.0f,
																								 stepRad * USceneManager::Get()->GetLastTickTime());
															  //LOG("SYSINPUT - CameraUP");
														  },
														  "GGCameraMove");

	UInputManager::Get()->SystemContext->BindActionSystem(CameraDown,
														  EKeyEvent::Repeat,
														  [&](const FKeyEventData& EventData) {
															  UCamera* Camera = USceneManager::Get()->GetActiveCamera();
															  CameraOrbitController->UpdateOrbit(Camera,
																								 0.0f,
																								 -stepRad * USceneManager::Get()->GetLastTickTime());
															  
														  },
														  "GCameraMove");

	UInputManager::Get()->SystemContext->BindActionSystem(CameraRight,
														  EKeyEvent::Repeat,
														  [&](const FKeyEventData& EventData) {
															  UCamera* Camera = USceneManager::Get()->GetActiveCamera();
															  CameraOrbitController->UpdateOrbit(Camera,
																								 stepRad * USceneManager::Get()->GetLastTickTime(),
																								 0.0f);
															  //LOG("SYSINPUT - CameraRight");
														  },
														  "GCameraMove");

	UInputManager::Get()->SystemContext->BindActionSystem(CameraLeft,
														  EKeyEvent::Repeat,
														  [&](const FKeyEventData& EventData) {
															  UCamera* Camera = USceneManager::Get()->GetActiveCamera();
															  CameraOrbitController->UpdateOrbit(Camera,
																								 -stepRad * USceneManager::Get()->GetLastTickTime(),
																								 0.0f);
														  },
														  "GCameraMove");
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
		//물리
		UPhysicsSystem::Get()->TickPhysics(DeltaTime);
		//리소스
		UResourceManager::Get()->Tick(DeltaTime);

		//일반 로직
		USceneManager::Get()->Tick(DeltaTime);
		UDebugDrawManager::Get()->Tick(DeltaTime);
#pragma endregion 
		
#pragma region Rendering
		//before render
		Renderer->BeginFrame(); 

		//SceneRender Submit
		USceneManager::Get()->Render(Renderer.get());
		UDebugDrawManager::Get()->Render(Renderer.get());

		//Actual Render 
		Renderer->ProcessRender();

#pragma region SystemUI
		UUIManager::Get()->RegisterUIElement([DeltaTime, &GameplayScene01, &GameplayScene02, &Renderer, &TestScene01]() {
			ImGui::SetNextWindowSize(ImVec2(400, 100));
			ImGui::SetNextWindowPos(ImVec2(SCREEN_WIDTH - 410, 0));
			ImGui::Begin("SystemUI", nullptr,
						 ImGuiWindowFlags_NoTitleBar |
						 ImGuiWindowFlags_NoResize |
						 ImGuiWindowFlags_NoMove |
						 ImGuiWindowFlags_NoBackground |
						 ImGuiWindowFlags_AlwaysAutoResize);
			if (ImGui::Button("Scene01"))
			{
				USceneManager::Get()->ChangeScene(GameplayScene01->GetName());
			}
			ImGui::SameLine();
			if (ImGui::Button("Scene02"))
			{
				USceneManager::Get()->ChangeScene(GameplayScene02->GetName());
			}
			ImGui::SameLine();
			if (ImGui::Button("Test01"))
			{
				USceneManager::Get()->ChangeScene(TestScene01->GetName());
			}
			ImGui::SameLine();
			if (ImGui::Button("CollisionTree")) {
				UPhysicsSystem::GetCollisionSubsystem()->PrintTreeStructure();
			}
			ImGui::SameLine();
			ImGui::Text("FPS : %d", (int)std::max(0.0f,1.0f / DeltaTime));
			
			if (ImGui::Button("PhysicsSystem")) {
				UPhysicsSystem::Get()->PrintDebugInfo();
			}

			if (ImGui::Button("RenderContext")) {
				Renderer->GetRenderContext()->PrintCurrentBindins();
			}
			ImGui::SameLine();
			if (ImGui::Button("Solid")) {
				Renderer->DefaultState = ERenderStateType::Solid;
			}
			ImGui::SameLine();
			if (ImGui::Button("WireFrame")) {
				Renderer->DefaultState = ERenderStateType::Wireframe;
			}
			ImGui::SameLine();
			if (ImGui::Button("LoadConfig")) {
				UConfigReadManager::Get()->LoadConfigFromIni();
			}
			//ImGui::Checkbox("ContextDebug", &(Renderer->GetRenderContext()->bDebugValidationEnabled));
			//ImGui::SameLine();
			//ImGui::Checkbox("ContextDebugBreak", &(Renderer->GetRenderContext()->bDebugBreakOnError));
			ImGui::End();
											 });

		UUIManager::Get()->RegisterUIElement([]()
											 {
												 constexpr ImGuiWindowFlags UIWindowFlags =
													 ImGuiWindowFlags_AlwaysAutoResize;  // 항상 내용에 맞게 크기 조절

												 auto Camera = USceneManager::Get()->GetActiveCamera();
												 if (Camera)
												 {
													 ImGui::Begin("Camera", nullptr, UIWindowFlags);
													 ImGui::Checkbox("bIs2D", &Camera->bIs2D);
													 ImGui::Checkbox("bLookAtObject", &Camera->bLookAtObject);
													 ImGui::Text(Debug::ToString(Camera->GetWorldTransform()));
													 ImGui::End();
												 }
											 });
#pragma endregion

		//UIRender
		USceneManager::Get()->RenderUI();
		UUIManager::Get()->RenderUI();

		//end render
		Renderer->EndFrame();


		//[LAST] reset Global FrameMemoryPool
		UFramePoolManager::Get()->EndFrame();
#pragma endregion

	}//end main Loop
#pragma endregion 

	// 여기에서 ImGui 소멸
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	GameplayScene01 = nullptr;
	GameplayScene02 = nullptr;
	Renderer = nullptr;
	RenderHardware = nullptr;

#pragma region COM
	CoUninitialize();
#pragma endregion
	return 0;
}

