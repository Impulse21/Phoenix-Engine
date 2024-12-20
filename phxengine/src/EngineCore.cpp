#include "phx/pch.h"
#include "phx/EngineCore.h"

#include <shellapi.h>  // For CommandLineToArgW
#include "phx/core/CommandLineArgs.h"
#include "phx/core/SystemTime.h"

#include "phx/rhi/GfxDevice.h"

#include "phx/Display.h"

#include "imgui.h"

#include <iostream>


#ifdef PHX_PLATFORM_WINDOWS

#include <DirectXMath.h>
#include <DirectXColors.h>
#endif

using namespace phx;
using namespace DirectX;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
void ExitGame() noexcept;


namespace phx::EngineCore
{
	HWND g_hWnd = nullptr; 
	HINSTANCE g_hInstance = nullptr;
}

namespace
{
	FILE* stream = nullptr;
	void ApplicationInitialize(IEngineApp& app)
	{
		int argc = 0;
		LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
		CommandLineArgs::Initialize(argc, argv);

		phx::Log::Initialize();
		
		SystemTime::Initialize();
		Display::Initialize({
				.WindowHandle = EngineCore::g_hWnd,
				.Width = 2000, // TODO: Data Drive this.
				.Height = 1200,
				.Format = rhi::Format::R10G10B10A2_UNORM,
				.Fullscreen = false,
				.VSync = false,
				.EnableHDR = false,
			});

		// TODO: Game Input
		// TODO: EngineTuning;
		app.Startup();
	}

	void ApplicationTick(IEngineApp& app)
	{
		// phx::EngineProfile::Update();
		app.Tick();
	}

	void ApplicationFinalize(IEngineApp& app)
	{
		// TODO: Idle GPU:
		app.Shutdown();

		Display::Finalize();
	}

	void ShowConsole()
	{
#if _DEBUG
		AllocConsole(); // Allocate a new console window
		freopen_s(&stream, "CONOUT$", "w", stdout);
		std::cout << "Console initialized." << std::endl;
#endif
	}
}

namespace phx::EngineCore
{
	int RunApplication(std::unique_ptr<IEngineApp>&& app, const wchar_t* className, HINSTANCE hInst, int nCmdShow)
	{
		if (!XMVerifyCPUSupport())
			return 1;

		BOOL dpi_success = SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
		assert(dpi_success);

		ShowConsole();

		// Register class and create window
		{
			// Register class
			WNDCLASSEXW wcex = {};
			wcex.cbSize = sizeof(WNDCLASSEXW);
			wcex.style = CS_HREDRAW | CS_VREDRAW;
			wcex.lpfnWndProc = WndProc;
			wcex.hInstance = hInst;
			wcex.hIcon = LoadIconW(hInst, L"IDI_ICON");
			wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
			wcex.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
			wcex.lpszClassName = L"PhxEditorWindowClass";
			wcex.hIconSm = LoadIconW(wcex.hInstance, L"IDI_ICON");
			if (!RegisterClassExW(&wcex))
				return 1;

			// Create window
			RECT rc = { 0, 0, (LONG)2000, (LONG)1700 };
			AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

			g_hWnd = CreateWindowExW(0, L"PhxEditorWindowClass", className, WS_OVERLAPPEDWINDOW,
				CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top,
				nullptr, nullptr, hInst,
				nullptr);
			// TODO: Change to CreateWindowExW(WS_EX_TOPMOST, L"PhxEditorWindowClass", g_szAppName, WS_POPUP,
			// to default to fullscreen.

			if (!g_hWnd)
				return 1;

			g_hInstance = hInst;
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
				ApplicationTick(*app);
			}
		}

		ApplicationFinalize(*app);
		app.reset();

		phx::rhi::GfxDevice::Finalize();

		if (stream)
		{
			fclose(stream);
		}
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
			ApplicationTick(*app);
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
		else if (s_in_sizemove)
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

			Display::Resize(static_cast<uint32_t>(rc.right - rc.left), static_cast<uint32_t>(rc.bottom - rc.top));
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

				// TODO
				SetWindowPos(hWnd, HWND_TOP, 0, 0, 2000, 1700, SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);
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

	ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam);

	return DefWindowProc(hWnd, message, wParam, lParam);
}

// Exit helper
void ExitGame() noexcept
{
	PostQuitMessage(0);
}
