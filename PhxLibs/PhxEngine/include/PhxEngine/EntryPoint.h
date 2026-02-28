#pragma once

#include <iostream>

#include <PhxCore/Base.h>
#include <PhxCore/StringUtils.h>
#include <PhxEngine/Application.h>
#include <PhxEngine/EngineCore.h>

#include <imgui.h>

#if false
#ifdef PHX_PLATFORM_WINDOWS

#define ENABLE_TRY_CATCH true

#if ENABLE_TRY_CATCH
#define BEGIN_TRY try{
#define END_TRY_AND_CATCH } catch (...) { PHX_CORE_ERROR("Exception has occured"); }
#else
#define BEGIN_TRY
#define END_TRY_AND_CATCH
#endif

extern "C" {
	_declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}

#ifdef PHX_RHI_D3D12

#include <d3d12.h>

extern "C"
{
	// Used to enable the "Agility SDK" components
	__declspec(dllexport) extern const UINT D3D12SDKVersion = D3D12_SDK_VERSION;
	__declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\";
}
#endif

#include <shellapi.h>  // For CommandLineToArgW

#include <WinSDKVer.h>
#define _WIN32_WINNT 0x0A00
#include <SDKDDKVer.h>

// Use the C++ standard templated min/max
#ifndef NOMINMAX
#define NOMINMAX
#endif
// DirectX apps don't need GDI
#define NODRAWTEXT
#define NOGDI
#define NOBITMAP

// Include <mcx.h> if you need this
#define NOMCX

// Include <winsvc.h> if you need this
#define NOSERVICE

// WinHelp is deprecated
#define NOHELP

#endif

#if defined(PHX_PLATFORM_WINDOWS)
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
void ExitGame() noexcept;

extern HWND g_hWnd;
extern HINSTANCE g_hInstance;

void ShowConsole(FILE* stream)
{
#if _DEBUG
	AllocConsole(); // Allocate a new console window
	freopen_s(&stream, "CONOUT$", "w", stdout);
	std::cout << "Console initialized." << std::endl;
#endif
}

// Window Procedure callback function
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam);

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPWSTR /*lpCmdLine*/, int nCmdShow)
{
#if false
	const wchar_t CLASS_NAME[] = L"PhoenixAppClassName";

	BOOL dpi_success = SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	assert(dpi_success);

	FILE* stream = nullptr;
	ShowConsole(stream);

	int argc = 0;
	LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);

	WNDCLASS wc = {};
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = CLASS_NAME;

	RegisterClass(&wc);

	// Sets up low level systems
	phx::EngineCore::PreInitialize(argc, argv);
	auto* app = phx::IApplication::Ptr;
	std::wstring appNameW;
	phx::StringConvert(app->GetName(), appNameW);

	// Create window
	uint32_t w, h;
	app->GetDefaultWindowSize(w, h);

	RECT rc = { 0, 0, static_cast<LONG>(w), static_cast<LONG>(h) };
	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

	HWND hwnd = CreateWindowEx(
		0,                          // Optional window styles
		CLASS_NAME,                 // Window class
		appNameW.c_str(),           // Window title
		WS_OVERLAPPEDWINDOW,        // Window style
		CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top,
		nullptr,                    // Parent window
		nullptr,                    // Menu
		hInstance,                  // Instance handle
		nullptr                     // Additional application data
	);

	if (hwnd == nullptr)
		return 0;

	ShowWindow(hwnd, nCmdShow);

	BEGIN_TRY
		// Sets up mid level systems
		phx::EngineCore::Initialize(hwnd);

		// Main message loop
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
				phx::EngineCore::Tick();
			}
		}
	END_TRY_AND_CATCH;

	phx::EngineCore::Finalize();
	if (stream)
	{
		fclose(stream);
	}

	return 0;
#else
{
    // Windows-only setup that can't move into core
    BOOL dpi_success = SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    assert(dpi_success);

    FILE* stream = nullptr;
    ShowConsole(stream);

    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    BEGIN_TRY
        phx::EngineCore::Run(argc, argv);
    END_TRY_AND_CATCH

    if (stream)
        fclose(stream);

    return 0;
#endif
}

// Exit helper
void ExitGame() noexcept
{
	PostQuitMessage(0);
}

#elif defined(PHX_PLATFORM_LINUX)
int main(int argc, char* argv[]) 
{
	phx::EngineCore::Run(argc, argv);

	return 0;
}

#else
	#error "Unsupported platform detected
#endif

#else
#if defined(PHX_PLATFORM_WINDOWS) ||defined(PHX_PLATFORM_LINUX)
int main(int argc, char* argv[])
{
	phx::EngineCore::Run(argc, argv);

	return 0;
}

#else
#error "Unsupported platform detected
#endif
#endif
