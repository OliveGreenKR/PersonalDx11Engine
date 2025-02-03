#include <windows.h>
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


#define KEY_UP 'W'
#define KEY_DOWN 'S'
#define KEY_LEFT 'A'
#define KEY_RIGHT 'D'

#define KEY_UP2 'I'
#define KEY_DOWN2 'K'
#define KEY_LEFT2 'J'
#define KEY_RIGHT2 'L'


const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 800;

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

	// 각종 메시지를 처리할 함수인 WndProc의 함수 포인터를 WindowClass 구조체에 넣는다.
	WNDCLASSW wndclass = { 0, WindowProc, 0, 0, 0, 0, 0, 0, 0, WindowClass };
	RegisterClassW(&wndclass);

	//윈도우 생성
	HWND hWnd = CreateWindowExW(0, WindowClass, Title, WS_POPUP | WS_VISIBLE | WS_OVERLAPPEDWINDOW,
								CW_USEDEFAULT, CW_USEDEFAULT, SCREEN_WIDTH, SCREEN_HEIGHT,
								nullptr, nullptr, hInstance, nullptr);
#pragma endregion


	//Renderer
	auto Renderer = make_unique<URenderer>();
	Renderer->Initialize(hWnd);
	Renderer->SetVSync(false);

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
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		//SemanticName, SemanticIndex, Foramt, InputSlot, AlignByteOffset, 
		// InputSlotClass,InstanceDataStepRate
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	//{
	//	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	//	{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	//};

	ID3D11SamplerState* SamplerState = Renderer->GetDefaultSamplerState();

	Shader->Initialize(Renderer->GetDevice(), MYSHADER, MYSHADER, layout, ARRAYSIZE(layout));
	Shader->Bind(Renderer->GetDeviceContext(), SamplerState);
	Shader->BindTexture(Renderer->GetDeviceContext(), *TDefault.get(), ETextureSlot::Albedo); //defaultTexure


	// 여기에서 ImGui를 생성합니다.
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	ImGui_ImplWin32_Init((void*)hWnd);
	ImGui_ImplDX11_Init(Renderer->GetDevice(), Renderer->GetDeviceContext());
	//ImGui set
	const float UIPaddingX = 10.0f / (float)SCREEN_WIDTH; //NDC Coordinates
	const float UIPaddingY = 10.0f / (float)SCREEN_HEIGHT;

	ImGui::SetNextWindowPos(ImVec2(UIPaddingX, UIPaddingY), ImGuiCond_FirstUseEver);//영구위치설정
	ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiCond_FirstUseEver); //자동조절

	const ImGuiWindowFlags UIWindowFlags =
		ImGuiWindowFlags_NoResize |      // 크기 조절 비활성화
		ImGuiWindowFlags_AlwaysAutoResize;  // 항상 내용에 맞게 크기 조절


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

	auto Camera = UCamera::Create(PI / 4.0f, SCREEN_WIDTH / SCREEN_HEIGHT, 0.1f, 100.0f);
	Camera->SetPosition({ 0,5.0f,-7.0f });
	

	//Main GameObejct
	auto Floor = UGameObject::Create(CubeModel);
	Floor->SetScale({ 5.0f,0.1f,5.0f });
	Floor->SetPosition({ 0,-1,0 });
	Floor->InitializePhysics();
	Floor->SetGravity(false);
	Floor->bIsPhysicsSimulated = false;

	auto Character = UGameObject::Create(CubeModel);
	Character->SetScale({ 0.5f,0.5f,0.5f });
	Character->SetPosition({ 0,0,0 });
	Character->InitializePhysics();
	Character->bIsPhysicsSimulated = true;

	auto Character2 = UGameObject::Create(SphereModel);
	Character2->SetScale({ 0.5f,0.5f,0.5f });
	Character2->SetPosition({ 0,0,0 });
	Character2->InitializePhysics();
	Character2->bIsPhysicsSimulated = true;

	Camera->SetLookAtObject(Character);
	Camera->bLookAtObject = false;
	Camera->LookTo(Character->GetTransform()->Position);

#pragma region  InputBind
	//input Action Bind - TODO::  Abstactionize 'Input Action'
	//현재는 객체가 직접 본인이 반응할 키 이벤트를 관리..
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
					Character->ApplyForce(Vector3::Forward * 100.0f);
					break;
				}
				case(ACTION_MOVE_DOWN_P1):
				{
					if (EventData.bShift)
					{
						Character->StartMove(-Vector3::Forward);
					}
					Character->ApplyForce(-Vector3::Forward * 100.0f);
					break;
				}
				case(ACTION_MOVE_RIGHT_P1):
				{
					if (EventData.bShift)
					{
						Character->StartMove(Vector3::Right);
					}
					Character->ApplyForce(Vector3::Right* 100.0f);
					break;
				}
				case(ACTION_MOVE_LEFT_P1):
				{
					if (EventData.bShift)
					{
						Character->StartMove(-Vector3::Right);
					}
					Character->ApplyForce(-Vector3::Right* 100.0f);
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
						Character2->ApplyForce(Vector3::Forward*100.0f);
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
				}
			}
		},
		"CharacterMove");



#pragma endregion

#pragma region MainLoop
	while (bIsExit == false)
	{
		MSG msg;

		//record deltaTime
		QueryPerformanceCounter(&currentTime);
		float deltaTime = static_cast<float>(currentTime.QuadPart - lastTime.QuadPart) / frequency.QuadPart;
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
		Character->Tick(deltaTime);
		Character2->Tick(deltaTime);
		Camera->Tick(deltaTime);
		//if (!Camera->IsInView(Character->GetTransform()->Position))
		//{
		//	Character->ApplyImpulse(-Vector3::Right * 5.0f);
		//}
#pragma endregion


#pragma region Rendering
		//before render
		Renderer->BeforeRender();

		//Render
		Renderer->RenderGameObject(Camera.get(),Character.get(), Shader.get(), *TTile.get());
		Renderer->RenderGameObject(Camera.get(),Character2.get(), Shader.get(), *TPole.get());
		Renderer->RenderGameObject(Camera.get(),Floor.get(), Shader.get(), *TRock.get());

#pragma region UI
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
		// ImGui UI 
		Vector3 CurrentVelo = Character->CurrentVelocity;

		ImGui::Begin("Camera", nullptr, UIWindowFlags);
		ImGui::Text("bIs2D : %d" , Camera->bIs2D);
		ImGui::Text("Position : %.2f  %.2f  %.2f", Camera->GetTransform()->Position.x,
					Camera->GetTransform()->Position.y,
					Camera->GetTransform()->Position.z);
		ImGui::Text("Rotation : %.2f  %.2f  %.2f", Camera->GetTransform()->GetEulerRotation().x,
					Camera->GetTransform()->GetEulerRotation().y,
					Camera->GetTransform()->GetEulerRotation().z);
		ImGui::End();

		ImGui::Begin("Charcter", nullptr, UIWindowFlags);
		ImGui::Text("FPS : %.2f", 1.0f / deltaTime);
		ImGui::Checkbox("bIsMove", &Character->bIsMoving);
		ImGui::Checkbox("bPhysicsBased", &Character->bIsPhysicsSimulated);
		ImGui::Text("CurrentVelo : %.2f  %.2f  %.2f", CurrentVelo.x,
					CurrentVelo.y,
					CurrentVelo.z);
		ImGui::Text("Position : %.2f  %.2f  %.2f", Character->GetTransform()->Position.x,
					Character->GetTransform()->Position.y,
					Character->GetTransform()->Position.z);
		ImGui::Text("Rotation : %.2f  %.2f  %.2f", Character->GetTransform()->GetEulerRotation().x,
					Character->GetTransform()->GetEulerRotation().y,
					Character->GetTransform()->GetEulerRotation().z);

		ImGui::End();

		ImGui::Begin("Charcter2", nullptr, UIWindowFlags);
		ImGui::Text("FPS : %.2f", 1.0f / deltaTime);
		ImGui::Checkbox("bIsMove", &Character2->bIsMoving);
		ImGui::Checkbox("bPhysicsBased", &Character2->bIsPhysicsSimulated);
		ImGui::Text("CurrentVelo : %.2f  %.2f  %.2f", CurrentVelo.x,
					CurrentVelo.y,
					CurrentVelo.z);
		ImGui::Text("Position : %.2f  %.2f  %.2f", Character2->GetTransform()->Position.x,
					Character2->GetTransform()->Position.y,
					Character2->GetTransform()->Position.z);
		ImGui::Text("Rotation : %.2f  %.2f  %.2f", Character2->GetTransform()->GetEulerRotation().x,
					Character2->GetTransform()->GetEulerRotation().y,
					Character2->GetTransform()->GetEulerRotation().z);

		ImGui::End();

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

