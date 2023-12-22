#include <PhxEngine/Assets/AssetFileManager.h>

#include <PhxEngine/Core/StopWatch.h>

using namespace PhxEngine;
using namespace PhxEngine::Assets;

namespace
{
	struct File
	{
		std::string Filename;
		std::unique_ptr<AssetFile> AssetFile;
	};

	enum class State
	{
		LoadingMetadata,
		ReadyToLoad,
		Loading,
		Loaded
	};

	State m_state = State::LoadingMetadata;
	Core::FlexArray<File> m_files;

	size_t loadedAssets;

	std::optional<Core::TimeStep> m_loadTime;

	std::shared_ptr<Core::IFileSystem> m_fileSystem;
	tf::Executor* m_executor;
}



void AssetFileManager::Initialize(tf::Executor* executor, std::shared_ptr<Core::IFileSystem> fileSystem)
{
	m_fileSystem = fileSystem;
	m_executor = executor;

}

