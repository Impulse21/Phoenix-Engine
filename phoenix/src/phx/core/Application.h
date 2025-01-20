#pragma once

#include <string>

namespace phx
{
	struct ApplicationSpecification
	{
		std::string Name = "Hazel Application";
		std::string WorkingDirectory;
	};

	class IApplication
	{
	public:
		inline static IApplication* Ptr = nullptr;

	public:
		virtual ~IApplication() = default;

		virtual void Tick() = 0;

		virtual void Startup() = 0;
		virtual void Shutdown() = 0;
	};

	// To be defined in CLIENT
	IApplication* CreateApplication();
}

