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

	ID3D11Buffer* vertexBufferTriangle = Renderer.CreateVertexBuffer(triangle_vertices, sizeof(triangle_vertices));
	ID3D11Buffer* vertexBufferCube = Renderer.CreateVertexBuffer(cube_vertices, sizeof(cube_vertices));
	ID3D11Buffer* vertexBufferSphere = Renderer.CreateVertexBuffer(sphere_vertices, sizeof(sphere_vertices));


	enum ETypePrimitive
	{
		EPT_Triangle,
		EPT_Cube,
		EPT_Sphere,
		EPT_Max,
	};

	ETypePrimitive typePrimitive = EPT_Triangle;

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
		
		switch (typePrimitive)
		{
			case EPT_Triangle:
				Renderer.RenderPrimitive(vertexBufferTriangle, numVerticesTriangle);
				break;
			case EPT_Cube:
				Renderer.RenderPrimitive(vertexBufferCube, numVerticesCube);
				break;
			case EPT_Sphere:
				Renderer.RenderPrimitive(vertexBufferSphere, numVerticesSphere);
				break;
		}


		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		// ImGui UI 컨트롤 추가
		ImGui::Begin("Jungle Property Window");
		ImGui::Text("Hello Jungle World!");

		if (ImGui::Button("Change primitive"))
		{
			switch (typePrimitive)
			{
				case EPT_Triangle:
					typePrimitive = EPT_Cube;
					break;
				case EPT_Cube:
					typePrimitive = EPT_Sphere;
					break;
				case EPT_Sphere:
					typePrimitive = EPT_Triangle;
					break;
			}
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

	Renderer.ReleaseVertexBuffer(vertexBufferTriangle);
	Renderer.ReleaseVertexBuffer(vertexBufferCube);
	Renderer.ReleaseVertexBuffer(vertexBufferSphere);
	Renderer.Shutdown();
	return 0;
}
