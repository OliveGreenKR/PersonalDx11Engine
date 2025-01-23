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
	ID3D11ShaderResourceView* DefaultTexture;
	assert(LoadTextureFromFile(Renderer->GetDevice(), TEXTURE01, &DefaultTexture), "Texture Load Failed");

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
	Shader->BindTexture(Renderer->GetDeviceContext(), DefaultTexture, ETextureSlot::Diffuset);


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

	auto Camera = make_unique<UCamera>(PI / 4.0f, SCREEN_WIDTH / SCREEN_HEIGHT, 0.1f, 1.0f);
	Camera->SetPosition({ 0,0,-5.0f });

	//Main GameObejct
	auto Character = make_shared<UGameObject>(TriModel);

#pragma region MainLoop
	while (bIsExit == false)
	{
		MSG msg;

		//record deltaTime
		QueryPerformanceCounter(&currentTime);
		float deltaTime = static_cast<float>(currentTime.QuadPart - lastTime.QuadPart) / frequency.QuadPart;
		lastTime = currentTime;
#pragma region Input
		//window msg process
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
			else if (msg.message == WM_KEYDOWN)//Key pushed
			{
				const float moveSpeed = 15.0f;
				const float roateSpeed = 30.0f;
				switch (msg.wParam)
				{
					//character
					case 'W':
					{
						Character->AddPosition({ 0,deltaTime * moveSpeed ,0 });
						break;
					}
					case 'S':
					{
						Character->AddPosition({ 0,-deltaTime * moveSpeed ,0 });
						break;
					}
					case 'D':
					{
						Character->AddPosition({ deltaTime * moveSpeed ,0,0 });
						break;
					}
					case 'A':
					{
						Character->AddPosition({ -deltaTime * moveSpeed ,0,0 });
						break;
					}
					//Camera
					case VK_UP:
					{
						//Pitch UP
						Camera->AddRotation({ Math::DegreeToRad(deltaTime * roateSpeed),0,0 });
						break;
					}
					case VK_DOWN:
					{
						Camera->AddRotation({ Math::DegreeToRad(-deltaTime * roateSpeed),0,0 });
						break;
					}
					case VK_RIGHT:
					{
						//Yaw Up
						Camera->AddRotation({ 0,Math::DegreeToRad(deltaTime * roateSpeed),0 });
						break;
					}
					case VK_LEFT:
					{
						Camera->AddRotation({ 0,Math::DegreeToRad(-deltaTime * moveSpeed),0 });
						break;
					}
				}
			}
		}

#pragma endregion

#pragma region logic
		if (Camera->IsInView(Character->GetTransform().Position) == false)
		{
			Character->SetPosition({ 0,0,0 });
		}
#pragma endregion


#pragma region Rendering
		//before render
		Renderer->BeforeRender();

		//Render
		//Renderer->RenderModel(TriModel.get(), Shader.get());
		Renderer->RenderGameObject(Camera.get(),Character.get(), Shader.get());

		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		// ImGui UI 
		ImGui::Begin("Property",nullptr, UIWindowFlags);
		ImGui::Text("FPS : %.2f", 1.0f / deltaTime );
		ImGui::Text("Position : %.2f  %.2f  %.2f", Character->GetTransform().Position.x,
					Character->GetTransform().Position.y,
					Character->GetTransform().Position.z);

		ImGui::End();

		ImGui::Render();
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

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

