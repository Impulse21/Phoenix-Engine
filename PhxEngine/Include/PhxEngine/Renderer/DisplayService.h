#pragma once

namespace PhxEngine
{
	class DisplayService
	{
	public:
		inline static DisplayService* Ptr;
		virtual ~DisplayService() = default;

	public:
		virtual void Startup() = 0;
		virtual void Shutdown() = 0;

		virtual void Update() = 0;

		virtual void Present() = 0;

		virtual void* GetNativeWindowHandle() = 0;
		virtual void* GetNativeWindow() = 0;
	};
}