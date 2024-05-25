#pragma once

namespace phx
{
	class IApplication
	{
	public:
		virtual void OnStartup() = 0;
		virtual void OnShutdown() = 0;

		virtual void OnPreRender() = 0;
		virtual void OnRender() = 0;
		virtual void OnUpdate() = 0;

		virtual bool EnableMultiThreading() = 0;

		virtual ~IApplication() = default;
	};
}

