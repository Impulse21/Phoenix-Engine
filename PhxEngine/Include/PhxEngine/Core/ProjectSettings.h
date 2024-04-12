#pragma once

#include <string>
#include <PhxEngine/Core/Object.h>
#include <filesystem>

namespace PhxEngine
{
	class ProjectSettings : public Object
	{
		PHX_OBJECT(ProjectSettings, Object);
	public:
		inline static ProjectSettings* Instance() { return sSingleton; }

	public:
		ProjectSettings();
		~ProjectSettings();

		bool Startup(std::string const& path);
		void Shutdown();

		const std::string& GetResourcePath() const { return this->m_resourcePath; }
		const std::string & GetName() const { return this->m_name; }
	private:
		bool LoadSettings(std::string const& path);
		
	private:
		std::string m_resourcePath;
		std::string m_name;
		std::string m_startingScene;

	private:
		inline static ProjectSettings* sSingleton = nullptr;
	};
}

