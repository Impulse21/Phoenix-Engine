#pragma once

#include <string>
#include <PhxEngine/Assets/AssetFile.h>
#include <PhxEngine/Core/VirtualFileSystem.h>
#include <PhxEngine/Core/FlexArray.h>
#include <taskflow/taskflow.hpp>

namespace PhxEngine::Assets
{
	namespace AssetFileManager
	{
		using FileHandle = size_t;

		void Initialize(tf::Executor* executor, std::shared_ptr<Core::IFileSystem> fileSystem);
		void OnUpdate();

		FileHandle RegisterFile(std::string const& filePath);

		void LoadNextSet(Core::FlexArray<FileHandle> handles);
		void UnloadSet();
		bool IsReadyToLoad();
		bool SetIsLoaded();

		bool IsLoading();
	};
}

