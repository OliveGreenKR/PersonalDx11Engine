#include <windows.h>
#include "Renderer.h"
#include "Math.h"
#include "D3D.h"
#include "Model.h"

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 800;


FVertexSimple triangle_vertices[] =
{
	{  0.0f,  1.0f, 0.0f,  1.0f, 0.0f, 0.0f, 1.0f }, // Top vertex (red)
	{  1.0f, -1.0f, 0.0f,  0.0f, 1.0f, 0.0f, 1.0f }, // Bottom-right vertex (green)
	{ -1.0f, -1.0f, 0.0f,  0.0f, 0.0f, 1.0f, 1.0f }  // Bottom-left vertex (blue)

};

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
	URenderer* Renderer = new URenderer();
	Renderer->Initialize(hWnd);
	Renderer->SetVSync(false);
	
	// ���⿡�� ImGui�� �����մϴ�.
	//IMGUI_CHECKVERSION();
	//ImGui::CreateContext();
	//ImGuiIO& io = ImGui::GetIO();
	//ImGui_ImplWin32_Init((void*)hWnd);
	//ImGui_ImplDX11_Init(Renderer.GetDevice(), Renderer.GetDeviceContext());

	bool bIsExit = false;

	//delta frame time
	LARGE_INTEGER frequency;
	LARGE_INTEGER lastTime;
	LARGE_INTEGER currentTime;

	QueryPerformanceFrequency(&frequency);
	QueryPerformanceCounter(&lastTime);

	FVertexSimple* vertices = triangle_vertices;
	UINT ByteWidth = sizeof(triangle_vertices);
	UINT numVertices = sizeof(triangle_vertices) / sizeof(FVertexSimple);

	UModel MSimpleTrianlgle = UModel::GetDefaultTriangle(Renderer->GetDevice());

#pragma region MainLoop
	while (bIsExit == false)
	{
		MSG msg;

		//record deltaTime
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

		}
		Renderer->BeforeRender();
		//Renderer->RenderPrimitive(vertexBuffer, numVertices);

		//Render
		Renderer->RenderModel(MSimpleTrianlgle);

		//ImGui_ImplDX11_NewFrame();
		//ImGui_ImplWin32_NewFrame();
		//ImGui::NewFrame();

		//// ImGui UI ��Ʈ�� �߰�
		//ImGui::Begin("Jungle Property Window");

		//ImGui::Text("Hello Jungle World!");
		//ImGui::Text("FPS : %.2f", 1.0f / deltaTime );

		//ImGui::End();

		//ImGui::Render();
		//ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		Renderer->EndRender();
	}
#pragma endregion
	// ���⿡�� ImGui �Ҹ�
	//ImGui_ImplDX11_Shutdown();
	//ImGui_ImplWin32_Shutdown();
	//ImGui::DestroyContext();

	Renderer->Release();
	return 0;
}

