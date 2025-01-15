#include <windows.h>
#include "Renderer.h"
#include "BasicShape.h"

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

	// 1024 x 1024 크기에 윈도우 생성
	HWND hWnd = CreateWindowExW(0, WindowClass, Title, WS_POPUP | WS_VISIBLE | WS_OVERLAPPEDWINDOW,
								CW_USEDEFAULT, CW_USEDEFAULT, 1024, 1024,
								nullptr, nullptr, hInstance, nullptr);
#pragma endregion
	bool bIsExit = false;

	URenderer Renderer;
	Renderer.Intialize(hWnd);

	// 여기에서 ImGui를 생성합니다.
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	ImGui_ImplWin32_Init((void*)hWnd);
	ImGui_ImplDX11_Init(Renderer.Device, Renderer.DeviceContext);


	//FVertexSimple* vertices = triangle_vertices;
	//UINT ByteWidth = sizeof(triangle_vertices);
	//UINT numVertices = sizeof(triangle_vertices) / sizeof(FVertexSimple);

	//FVertexSimple* vertices = cube_vertices;
	//UINT ByteWidth = sizeof(cube_vertices);
	//UINT numVertices = sizeof(cube_vertices) / sizeof(FVertexSimple);

	FVertexSimple* vertices = sphere_vertices;
	UINT ByteWidth = sizeof(sphere_vertices);
	UINT numVertices = sizeof(sphere_vertices) / sizeof(FVertexSimple);

	float scaleMod = 0.1f;
	for (UINT i = 0; i < numVertices; ++i)
	{
		sphere_vertices[i].x *= scaleMod;
		sphere_vertices[i].y *= scaleMod;
		sphere_vertices[i].z *= scaleMod;
	}

	// 생성
	D3D11_BUFFER_DESC vertexbufferdesc = {};
	vertexbufferdesc.ByteWidth = ByteWidth;
	vertexbufferdesc.Usage = D3D11_USAGE_IMMUTABLE;
	vertexbufferdesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA vertexbufferSRD = { vertices };

	ID3D11Buffer* vertexBuffer;

	Renderer.Device->CreateBuffer(&vertexbufferdesc, &vertexbufferSRD, &vertexBuffer);

#pragma region MainLoop
	while (bIsExit == false)
	{
		MSG msg;

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

	
		Renderer.PrepareRender();
		Renderer.PrepareShader();
		Renderer.RenderPrimitive(vertexBuffer, numVertices);

		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		// ImGui UI 컨트롤 추가
		ImGui::Begin("Jungle Property Window");
		ImGui::Text("Hello Jungle World!");

		if (ImGui::Button("Quit this app"))
		{
			// 현재 윈도우에 Quit 메시지를 메시지 큐로 보냄
			PostMessage(hWnd, WM_QUIT, 0, 0);
		}

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

	vertexBuffer->Release();
	Renderer.Shutdown();
	return 0;
}
