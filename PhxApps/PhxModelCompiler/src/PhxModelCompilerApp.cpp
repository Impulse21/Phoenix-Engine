#include <PhxCore/Base.h>
#include <PhxCore/StringUtils.h>

#include <PhxEngine/JobSystem.h>

using namespace phx;

int wmain(int argc, wchar_t** argv)
{
	phx::Log::Initialize();
	if (argc == 0)
	{
		PHX_INFO("JSON config expected");
		return -1;
	}

	phx::JobSystem::Initialize();

	std::wstring wConfigFile = argv[1];
	std::string configFile;
	StringConvert(wConfigFile, configFile);


	phx::JobSystem::Shutdown();

}