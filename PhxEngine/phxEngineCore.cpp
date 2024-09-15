#include "pch.h"
#include "phxEngineCore.h"

#include "phxCommandLineArgs.h"
#include "phxDisplay.h"
#include "phxGfxCore.h"
#include <shellapi.h>  // For CommandLineToArgW

#include "phxDeferredReleaseQueue.h"
#include "phxSystemTime.h"

#ifdef __clang__
#pragma clang diagnostic ignored "-Wcovered-switch-default"
#pragma clang diagnostic ignored "-Wswitch-enum"
#endif

using namespace phx;
using namespace DirectX;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void ExitGame() noexcept;


namespace
{
	void ApplicationInitialize(IEngineApp& app)
	{
		int argc = 0;
		LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
		CommandLineArgs::Initialize(argc, argv);

		gfx::Initialize();
		SystemTime::Initialize();
		// TODO: Game Input
		// TODO: EngineTuning;
		app.Startup();
	}

	void UpdateApplication(IEngineApp& app)
	{
		Display::Preset();
	}

	void ApplicationFinalize(IEngineApp& app)
	{
		// TODO: Idle GPU;

		app.Shutdonw();

		DeferredDeleteQueue::ReleaseItems();
	}
}

namespace phx::EngineCore
{

	HWND g_hWnd = nullptr;
	int RunApplication(std::unique_ptr<IEngineApp>&& app, const wchar_t* className, HINSTANCE hInst, int nCmdShow)
	{
		if (!XMVerifyCPUSupport())
			return 1;

		// Initialize the GameRuntime
		HRESULT hr = XGameRuntimeInitialize();
		if (FAILED(hr))
		{
			if (hr == E_GAMERUNTIME_DLL_NOT_FOUND || hr == E_GAMERUNTIME_VERSION_MISMATCH)
			{
				std::ignore = MessageBoxW(nullptr, L"Game Runtime is not installed on this system or needs updating.", className, MB_ICONERROR | MB_OK);
			}
			return 1;
		}


		// Register class and create window
		{
			// Register class
			WNDCLASSEXW wcex = {};
			wcex.cbSize = sizeof(WNDCLASSEXW);
			wcex.style = CS_HREDRAW | CS_VREDRAW;
			wcex.lpfnWndProc = WndProc;
			wcex.hInstance = hInst;
			wcex.hIcon = LoadIconW(hInst, L"IDI_ICON");
			wcex.hCursor = LoadCursorW(nullptr, IDC_ARROW);
			wcex.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
			wcex.lpszClassName = L"PhxEditorWindowClass";
			wcex.hIconSm = LoadIconW(wcex.hInstance, L"IDI_ICON");
			if (!RegisterClassExW(&wcex))
				return 1;

			// Create window
			RECT rc = { 0, 0, (LONG)gfx::g_DisplayWidth, (LONG)gfx::g_DisplayHeight };
			AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

			g_hWnd = CreateWindowExW(0, L"PhxEditorWindowClass", className, WS_OVERLAPPEDWINDOW,
				CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top,
				nullptr, nullptr, hInst,
				nullptr);
			// TODO: Change to CreateWindowExW(WS_EX_TOPMOST, L"PhxEditorWindowClass", g_szAppName, WS_POPUP,
			// to default to fullscreen.

			if (!g_hWnd)
				return 1;

			ApplicationInitialize(*app);

			ShowWindow(g_hWnd, nCmdShow/*SW_SHOWDEFAULT*/);
			// TODO: Change nCmdShow to SW_SHOWMAXIMIZED to default to fullscreen.
		}

		// Main message loop
		MSG msg = {};
		while (WM_QUIT != msg.message)
		{
			if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else
			{
				UpdateApplication(*app);
			}
		}

		ApplicationFinalize(*app);
		app.reset();

		XGameRuntimeUninitialize();
		gfx::Finalize();

		return static_cast<int>(msg.wParam);
	}
}



// Windows procedure
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static bool s_in_sizemove = false;
	static bool s_in_suspend = false;
	static bool s_minimized = false;
	static bool s_fullscreen = false;
	// TODO: Set s_fullscreen to true if defaulting to fullscreen.

	auto app = reinterpret_cast<IEngineApp*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

	switch (message)
	{
	case WM_CREATE:
		if (lParam)
		{
			auto params = reinterpret_cast<LPCREATESTRUCTW>(lParam);
			SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(params->lpCreateParams));
		}
		break;

	case WM_PAINT:
		if (s_in_sizemove && app)
		{
		EngineCore:UpdateApplication(*app);
		}
		else
		{
			PAINTSTRUCT ps;
			std::ignore = BeginPaint(hWnd, &ps);
			EndPaint(hWnd, &ps);
		}
		break;

	case WM_DISPLAYCHANGE:
		if (app)
		{
			// app->OnDisplayChange();
		}
		break;

	case WM_MOVE:
		if (app)
		{
			// app->OnWindowMoved();
		}
		break;

	case WM_SIZE:
		if (wParam == SIZE_MINIMIZED)
		{
			if (!s_minimized)
			{
				s_minimized = true;
				if (!s_in_suspend && app)
					// app->OnSuspending();
				s_in_suspend = true;
			}
		}
		else if (s_minimized)
		{
			s_minimized = false;
			if (s_in_suspend && app)
				// app->OnResuming();
			s_in_suspend = false;
		}
		else if (!s_in_sizemove && app)
		{
			Display::Resize(LOWORD(lParam), HIWORD(lParam));
		}

		break;

	case WM_ENTERSIZEMOVE:
		s_in_sizemove = true;
		break;

	case WM_EXITSIZEMOVE:
		s_in_sizemove = false;
		if (app)
		{
			RECT rc;
			GetClientRect(hWnd, &rc);

			// app->OnWindowSizeChanged(rc.right - rc.left, rc.bottom - rc.top);
		}
		break;

	case WM_GETMINMAXINFO:
		if (lParam)
		{
			auto info = reinterpret_cast<MINMAXINFO*>(lParam);
			info->ptMinTrackSize.x = 320;
			info->ptMinTrackSize.y = 200;
		}
		break;

	case WM_ACTIVATEAPP:
#if false
		if (app)
		{
			if (wParam)
			{
				app->OnActivated();
			}
			else
			{
				app->OnDeactivated();
			}
		}
#endif
		break;

	case WM_POWERBROADCAST:
#if false
		switch (wParam)
		{
		case PBT_APMQUERYSUSPEND:
			if (!s_in_suspend && app)
				app->OnSuspending();
			s_in_suspend = true;
			return TRUE;

		case PBT_APMRESUMESUSPEND:
			if (!s_minimized)
			{
				if (s_in_suspend && app)
					app->OnResuming();
				s_in_suspend = false;
			}
			return TRUE;
		}
#endif
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_SYSKEYDOWN:
		if (wParam == VK_RETURN && (lParam & 0x60000000) == 0x20000000)
		{
			// Implements the classic ALT+ENTER fullscreen toggle
			if (s_fullscreen)
			{
				SetWindowLongPtr(hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);
				SetWindowLongPtr(hWnd, GWL_EXSTYLE, 0);


				ShowWindow(hWnd, SW_SHOWNORMAL);

				SetWindowPos(hWnd, HWND_TOP, 0, 0, gfx::g_DisplayWidth, gfx::g_DisplayHeight, SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);
			}
			else
			{
				SetWindowLongPtr(hWnd, GWL_STYLE, WS_POPUP);
				SetWindowLongPtr(hWnd, GWL_EXSTYLE, WS_EX_TOPMOST);

				SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

				ShowWindow(hWnd, SW_SHOWMAXIMIZED);
			}

			s_fullscreen = !s_fullscreen;
		}
		break;

	case WM_MENUCHAR:
		// A menu is active and the user presses a key that does not correspond
		// to any mnemonic or accelerator key. Ignore so we don't produce an error beep.
		return MAKELRESULT(0, MNC_CLOSE);
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

// Exit helper
void ExitGame() noexcept
{
	PostQuitMessage(0);
}
