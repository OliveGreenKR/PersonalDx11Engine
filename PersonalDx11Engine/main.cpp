#include <windows.h>
#include "Renderer.h"

//struct for Processing Win Msgs
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
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

	// 1024 x 1024 ũ�⿡ ������ ����
	HWND hwnd = CreateWindowExW(0, WindowClass, Title, WS_POPUP | WS_VISIBLE | WS_OVERLAPPEDWINDOW,
								CW_USEDEFAULT, CW_USEDEFAULT, 1024, 1024,
								nullptr, nullptr, hInstance, nullptr);

#pragma endregion
	bool bIsExit = false;

	URenderer Renderer;
	Renderer.Intialize(hwnd);


	FVertexSimple* vertices = triangle_vertices;
	UINT ByteWidth = sizeof(triangle_vertices);
	UINT numVertices = sizeof(triangle_vertices) / sizeof(FVertexSimple);

	// ����
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

	
		Renderer.PrepareRender();
		Renderer.PrepareShader();
		Renderer.RenderPrimitive(vertexBuffer, numVertices);

		Renderer.SwapBuffer();
	}
#pragma endregion

	vertexBuffer->Release();
	Renderer.Shutdown();
	return 0;
}
