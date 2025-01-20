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
		virtual void Tick() = 0;
	};

	// To be defined in CLIENT
	IApplication* CreateApplication();
}

