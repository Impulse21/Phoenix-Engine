#pragma once

#include <pch.h>

#include <memory>

namespace phx
{
	class IEngineApp
	{
	public:
		virtual ~IEngineApp() = default;

		virtual void Startup() = 0;
		virtual void Shutdown() = 0;

		virtual void CacheRenderData() = 0;
		virtual void Update() = 0;
		virtual void Render() = 0;
	};

	namespace EngineCore
	{
		int RunApplication(std::unique_ptr<IEngineApp>&& app, const wchar_t* className, HINSTANCE hInst, int nCmdShow );
	}
}


#define CREATE_APPLICATION( app_class ) \
    int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE /*hPrevInstance*/, _In_ LPWSTR /*lpCmdLine*/, _In_ int nCmdShow) \
    { \
        return phx::EngineCore::RunApplication( std::make_unique<app_class>(), L#app_class, hInstance, nCmdShow ); \
    }
