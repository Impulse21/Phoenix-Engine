#pragma once

#include <iostream>

#include "phx/core/Base.h"
#include "phx/core/Application.h"
#include "phx/core/CommandLineArgs.h"
#include "phx/core/StringUtils.h"

#include "imgui.h"

#ifdef PHX_PLATFORM_WINDOWS

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

#ifdef PHX_PLATFORM_WINDOWS
extern phx::IApplication* phx::CreateApplication();

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
void ExitGame() noexcept;

HWND g_hWnd;
HINSTANCE g_hInstance;

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

	default:
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	const wchar_t CLASS_NAME[] = L"PhoenixAppClassName";

	BOOL dpi_success = SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	assert(dpi_success);

	FILE* stream = nullptr;
	ShowConsole(stream);

	phx::Log::Initialize();

	int argc = 0;
	LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	phx::CommandLineArgs::Initialize(argc, argv);

	phx::IApplication::Ptr = phx::CreateApplication();
	auto* app = phx::IApplication::Ptr;

	WNDCLASS wc = {};
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = CLASS_NAME;

	RegisterClass(&wc);

	std::wstring appNameW;
	phx::StringConvert(app->GetName(), appNameW);

	HWND hwnd = CreateWindowEx(
		0,                          // Optional window styles
		CLASS_NAME,                 // Window class
		appNameW.c_str(),           // Window title
		WS_OVERLAPPEDWINDOW,        // Window style
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		nullptr,                    // Parent window
		nullptr,                    // Menu
		hInstance,                  // Instance handle
		nullptr                     // Additional application data
	);

	if (hwnd == nullptr)
		return 0;

	ShowWindow(hwnd, nCmdShow);

	phx::IApplication::Ptr->Startup();

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
			phx::IApplication::Ptr->Tick();
		}
	}


	phx::IApplication::Ptr->Shutdown();

	delete phx::IApplication::Ptr;
	phx::IApplication::Ptr = nullptr;

	if (stream)
	{
		fclose(stream);
	}

	return 0;
}

// Exit helper
void ExitGame() noexcept
{
	PostQuitMessage(0);
}

#endif
