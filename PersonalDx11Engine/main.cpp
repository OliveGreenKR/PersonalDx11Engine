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
	// ������ Ŭ���� �̸�
	WCHAR WindowClass[] = L"JungleWindowClass";

	// ������ Ÿ��Ʋ�ٿ� ǥ�õ� �̸�
	WCHAR Title[] = L"Game Tech Lab";

	// ���� �޽����� ó���� �Լ��� WndProc�� �Լ� �����͸� WindowClass ����ü�� �ִ´�.
	WNDCLASSW wndclass = { 0, WindowProc, 0, 0, 0, 0, 0, 0, 0, WindowClass };

	// ������ Ŭ���� ���
	RegisterClassW(&wndclass);

	//������ ����
	HWND hWnd = CreateWindowExW(0, WindowClass, Title, WS_POPUP | WS_VISIBLE | WS_OVERLAPPEDWINDOW,
								CW_USEDEFAULT, CW_USEDEFAULT, 800, 800,
								nullptr, nullptr, hInstance, nullptr);
#pragma endregion
	URenderer Renderer;
	Renderer.Intialize(hWnd);
	Renderer.bVSync = 0;
	

	// ���⿡�� ImGui�� �����մϴ�.
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
			// Ű �Է� �޽����� ����
			TranslateMessage(&msg);
			// �޽����� ������ ������ ���ν����� ����
			DispatchMessage(&msg);

			if (msg.message == WM_QUIT)
			{
				bIsExit = true;
				break;
			}

			if (msg.message == WM_KEYDOWN) // Ű���� ������ ��
			{
				const float unitmove = 0.01f;
				// ���� Ű�� ����Ű��� �ش� ���⿡ ���缭
				// offset ������ x, y �ɹ� ������ ���� �����մϴ�.
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

			//���� �浹�� ���� ��ȯ
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

		// ImGui UI ��Ʈ�� �߰�
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


	// ���⿡�� ImGui �Ҹ�
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	Renderer.ReleaseVertexBuffer(vertexBufferSphere);
	Renderer.Shutdown();
	return 0;
}

