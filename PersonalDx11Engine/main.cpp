#include <windows.h>
#include <iostream>

//ImGui
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_impl_dx11.h"
#include "imGui/imgui_impl_win32.h"

#include "Debug.h"
#include "DebugDrawManager.h"
#include <memory>
#include "Renderer.h"
#include "Math.h"
#include "D3D.h"
#include "D3DShader.h"
#include "InputManager.h"

#include "Color.h"
#include "define.h"
#include "CollisionManager.h"

#include "SceneManager.h"
#include "GameplayScene01.h"
#include "GameplayScene02.h"

#include "ResourceManager.h"
#include "UIManager.h"

//test
#include "testDynamicAABBTree.h"
#include "testSceneComponent.h"

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
void LimitFrameRate(float targetFPS)
{
	static LARGE_INTEGER frequency;
	static LARGE_INTEGER lastTime;
	LARGE_INTEGER currentTime;

	// 첫 호출 시 초기화
	if (frequency.QuadPart == 0)
	{
		QueryPerformanceFrequency(&frequency);
		QueryPerformanceCounter(&lastTime);
	}

	// 프레임당 필요한 시간 계산
	float frameTime = 1.0f / targetFPS;

	// 현재 시간 측정
	QueryPerformanceCounter(&currentTime);

	// 경과 시간 계산
	float elapsedTime =
		static_cast<float>(currentTime.QuadPart - lastTime.QuadPart) / frequency.QuadPart;

	// 남은 시간만큼 대기
	if (elapsedTime < frameTime)
	{
		DWORD sleepTime = static_cast<DWORD>((frameTime - elapsedTime) * 1000.0f);
		Sleep(sleepTime);
	}

	// 마지막 시간 업데이트
	lastTime = currentTime;
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

	//TestDynamicAABBTree::RunAllTestsIterate(std::cout, 100, 500, 500, 500);
	//std::string input;
	//std::getline(std::cin, input); // 사용자 입력을 기다림
	//return 0;

	//TestSceneComponent::RunTransformTest(std::cout, 20, 3);
	//std::string input;
	//std::getline(std::cin, input); // 사용자 입력을 기다림
	//return 0;

	//Hardware
	auto RenderHardware = make_shared<FD3D>();
	assert(RenderHardware->Initialize(hWnd));
	RenderHardware->SetVSync(false);

	//Renderer
	auto Renderer = make_unique<URenderer>();
	Renderer->Initialize(hWnd, RenderHardware.get());


	//ModelBufferManager Init
	UModelBufferManager::Get()->SetDevice(Renderer->GetDevice());
	assert(UModelBufferManager::Get()->Initialize());

	//ResourceManager
	UResourceManager::Get()->Initialize(RenderHardware.get());

	//Shader
	D3D11_INPUT_ELEMENT_DESC textureShaderLayout[] =
	{
		//SemanticName, SemanticIndex, Foramt, InputSlot, AlignByteOffset, 
		// InputSlotClass,InstanceDataStepRate
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	auto Shader = UResourceManager::Get()->LoadShader(MYSHADER, MYSHADER, textureShaderLayout, ARRAYSIZE(textureShaderLayout));

	ID3D11SamplerState* SamplerState = Renderer->GetDefaultSamplerState();
	Shader->Bind(Renderer->GetDeviceContext(), SamplerState);
	
	//UIManager
	UUIManager::Get()->Initialize(hWnd, RenderHardware->GetDevice(), RenderHardware->GetDeviceContext());

	bool bIsExit = false;

	//delta frame time
	LARGE_INTEGER frequency;
	LARGE_INTEGER lastTime;
	LARGE_INTEGER currentTime;

	QueryPerformanceFrequency(&frequency);
	QueryPerformanceCounter(&lastTime);

	//Scene
	auto GameplayScene01 = make_shared<UGameplayScene01>();
	USceneManager::Get()->RegisterScene(GameplayScene01);

	auto GameplayScene02 = make_shared<UGameplayScene02>();
	USceneManager::Get()->RegisterScene(GameplayScene02);

	//Defualt Scene Load
	USceneManager::Get()->ChangeScene(GameplayScene02->GetName());

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
		UDebugDrawManager::Get()->Tick(DeltaTime);
		USceneManager::Get()->Tick(DeltaTime);
		UCollisionManager::Get()->Tick(DeltaTime);
#pragma endregion 
		
#pragma region Rendering
		//before render
		Renderer->BeforeRender();

		USceneManager::Get()->Render(Renderer.get());

		//Actual Render 
		Renderer->ProcessRenderJobs(Shader.get());
		Renderer->ClearRenderJobs();

#pragma region SystemUI
		UUIManager::Get()->RegisterUIElement("SystemUI", [DeltaTime, &GameplayScene01, &GameplayScene02]() {
			ImGui::SetNextWindowSize(ImVec2(400, 30));
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
			if (ImGui::Button("CollisionTree")) {
				UCollisionManager::Get()->PrintTreeStructure();
			}
			ImGui::SameLine();
			ImGui::Text("FPS : %.0f", std::max(0.0f,1.0f / DeltaTime));
			ImGui::End();
											 });

#pragma endregion

		USceneManager::Get()->RenderUI();
		UUIManager::Get()->RenderUI();

		auto Camera = USceneManager::Get()->GetActiveCamera();
		UDebugDrawManager::Get()->DrawAll(Camera);

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

