#include <windows.h>
#include "Renderer.h"
#include "BasicShape.h"
#include "Math.h"
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
#pragma region window init
	// 윈도우 클래스 이름
	WCHAR WindowClass[] = L"JungleWindowClass";

	// 윈도우 타이틀바에 표시될 이름
	WCHAR Title[] = L"Game Tech Lab";

	// 각종 메시지를 처리할 함수인 WndProc의 함수 포인터를 WindowClass 구조체에 넣는다.
	WNDCLASSW wndclass = { 0, WindowProc, 0, 0, 0, 0, 0, 0, 0, WindowClass };

	// 윈도우 클래스 등록
	RegisterClassW(&wndclass);

	//윈도우 생성
	HWND hWnd = CreateWindowExW(0, WindowClass, Title, WS_POPUP | WS_VISIBLE | WS_OVERLAPPEDWINDOW,
								CW_USEDEFAULT, CW_USEDEFAULT, 800, 800,
								nullptr, nullptr, hInstance, nullptr);
#pragma endregion
	URenderer Renderer;
	Renderer.Intialize(hWnd);
	Renderer.bVSync = 0;
	

	// 여기에서 ImGui를 생성합니다.
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	ImGui_ImplWin32_Init((void*)hWnd);
	ImGui_ImplDX11_Init(Renderer.Device, Renderer.DeviceContext);

	UINT numVerticesTriangle = sizeof(triangle_vertices) / sizeof(FVertexSimple);
	UINT numVerticesCube = sizeof(cube_vertices) / sizeof(FVertexSimple);
	UINT numVerticesSphere = sizeof(sphere_vertices) / sizeof(FVertexSimple);

	float scaleMod = 0.1f;
	for (UINT i = 0; i < numVerticesSphere; ++i)
	{
		sphere_vertices[i].x *= scaleMod;
		sphere_vertices[i].y *= scaleMod;
		sphere_vertices[i].z *= scaleMod;
	}

	/*ID3D11Buffer* vertexBufferTriangle = Renderer.CreateVertexBuffer(triangle_vertices, sizeof(triangle_vertices));
	ID3D11Buffer* vertexBufferCube = Renderer.CreateVertexBuffer(cube_vertices, sizeof(cube_vertices));*/
	ID3D11Buffer* vertexBufferSphere = Renderer.CreateVertexBuffer(sphere_vertices, sizeof(sphere_vertices));

	ETypePrimitive typePrimitive = ETypePrimitive::Sphere;

	bool bIsExit = false;
	FVector offset;
	FVector velocity;

	const float leftBorder = -1.0f;
	const float rightBorder = 1.0f;
	const float topBorder = -1.0f;
	const float bottomBorder = 1.0f;
	const float sphereRadius = 1.0f;

	bool bBoundBallToScreen = true;
	bool bPinballMovement = true;

	velocity.x = ((float)(rand() % 100 - 50)) * 0.05f;
	velocity.y = ((float)(rand() % 100 - 50)) * 0.05f;


	//delta frame time
	LARGE_INTEGER frequency;
	LARGE_INTEGER lastTime;
	LARGE_INTEGER currentTime;

	QueryPerformanceFrequency(&frequency);
	QueryPerformanceCounter(&lastTime);

#pragma region MainLoop
	while (bIsExit == false)
	{
		MSG msg;

		QueryPerformanceCounter(&currentTime);
		float deltaTime = static_cast<float>(currentTime.QuadPart - lastTime.QuadPart) / frequency.QuadPart;
		lastTime = currentTime;

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

			if (msg.message == WM_KEYDOWN) // 키보드 눌렸을 때
			{
				const float unitmove = 0.01f;
				// 눌린 키가 방향키라면 해당 방향에 맞춰서
				// offset 변수의 x, y 맴버 변수의 값을 조정합니다.
				if (msg.wParam == VK_LEFT)
				{
					offset.x -= unitmove;
				}
				if (msg.wParam == VK_RIGHT)
				{
					offset.x += unitmove;
				}
				if (msg.wParam == VK_UP)
				{
					offset.y += unitmove;
				}
				if (msg.wParam == VK_DOWN)
				{
					offset.y -= unitmove;
				}

				if (bBoundBallToScreen)
				{
					if (msg.wParam == VK_LEFT)
					{
						offset.x = clamp<float>(offset.x, leftBorder, rightBorder);
					}
					if (msg.wParam == VK_RIGHT)
					{
						offset.x = clamp<float>(offset.x, leftBorder, rightBorder);
					}
					if (msg.wParam == VK_UP)
					{
						offset.y = clamp<float>(offset.y, leftBorder, rightBorder);
					}
					if (msg.wParam == VK_DOWN)
					{
						offset.y = clamp<float>(offset.y, leftBorder, rightBorder);
					}
				}
				
			}
		}

		if (bPinballMovement)
		{
			offset += velocity * deltaTime;

			//경계면 충돌시 방향 전환
			float renderRadius = sphereRadius * scaleMod;
			if (offset.x < leftBorder + renderRadius)
			{
				velocity.x *= -1.0f;
			}
			if (offset.x > rightBorder - renderRadius)
			{
				velocity.x *= -1.0f;
			}
			if (offset.y < topBorder + renderRadius)
			{
				velocity.y *= -1.0f;
			}
			if (offset.y > bottomBorder - renderRadius)
			{
				velocity.y *= -1.0f;
			}

		}

	
		Renderer.PrepareRender();
		Renderer.PrepareShader();

		Renderer.UpdateConstant(offset);

		Renderer.RenderPrimitive(vertexBufferSphere, numVerticesSphere);

		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		// ImGui UI 컨트롤 추가
		ImGui::Begin("Jungle Property Window");
		ImGui::Text("Hello Jungle World!");
		ImGui::Text("FPS : %.2f", 1.0f / deltaTime );
		ImGui::Text("radius : %.6f", sphereRadius * scaleMod);
		ImGui::Text("velocity : %.2f ,  %0.2f", velocity.x, velocity.y);
		ImGui::Text("position : %.2f ,  %0.2f", offset.x, offset.y);


		ImGui::Checkbox("Bound Ball To Screen", &bBoundBallToScreen);
		ImGui::Checkbox("Pinball Movement", &bPinballMovement);

		ImGui::End();

		ImGui::Render();
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		Renderer.SwapBuffer();
	}
#pragma endregion


	// 여기에서 ImGui 소멸
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	Renderer.ReleaseVertexBuffer(vertexBufferSphere);
	Renderer.Shutdown();
	return 0;
}

