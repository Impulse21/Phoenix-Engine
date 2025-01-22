#pragma once

#include <string>

namespace phx
{
	struct ApplicationDescriptor
	{
		std::string Name = "Phoenix Application";
		std::string WorkingDirectory = "";
		uint32_t Width = 1600;
		uint32_t Height = 900;
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

		virtual const char* GetName() const = 0;
		virtual void GetDefaultWindowSize(uint32_t& outWidth, uint32_t& outHeight) const = 0;
	};

	// To be defined in CLIENT
	IApplication* CreateApplication();
}

