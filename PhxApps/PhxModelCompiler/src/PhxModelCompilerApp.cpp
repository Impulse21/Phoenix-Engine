#include <PhxCore/Base.h>
#include <PhxCore/CommandLineArgs.h>

int wmain(int argc, wchar_t** argv)
{
	phx::Log::Initialize();
	if (argc == 0)
	{
		PHX_INFO("YAML config expected");
		return -1;
	}

	phx::CommandLineArgs::Initialize(argc, argv);
	phx::ThreadPool::Initialize();

	std::wstring wConfigFile;
	phx::CommandLineArgs::GetString(L"config", wConfigFile);

	std::string configFile;
	StringConvert(wConfigFile, configFile);

}