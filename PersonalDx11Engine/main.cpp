#include <windows.h>
#include "D3DRenderer.h"

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

	FD3DRenderer Renderer;
	Renderer.Intialize(hwnd);


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

		//Tick
		Renderer.Tick();

	}
#pragma endregion


	Renderer.Shutdown();
	return 0;
}
