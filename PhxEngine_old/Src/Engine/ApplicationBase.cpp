#include "C:/Users/dipao/source/repos/Impulse21/Phoenix-Engine/Build/PhxEngine/CMakeFiles/PhxEngine.dir/Debug/cmake_pch.hxx"
#include <PhxEngine/Engine/ApplicationBase.h>
#include <PhxEngine/Core/VirtualFileSystem.h>
#include <PhxEngine/Graphics/TextureCache.h>
#include <taskflow/taskflow.hpp>  // Taskflow is header-only

void PhxEngine::ApplicationBase::Render()
{
	if (!this->m_sceneLoaded.load())
	{
		this->RenderSplashScreen();
		return;
	}

	if (this->m_sceneLoaded.load())
	{
		if (this->m_sceneLoadingThread)
		{
			this->m_sceneLoadingThread->join();
			this->m_sceneLoadingThread.reset();
		}

		this->SceneLoaded();
	}

	this->RenderScene();
}

void PhxEngine::ApplicationBase::BeginLoadingScene(std::shared_ptr<Core::IFileSystem> fileSystem, const std::filesystem::path& sceneFileName)
{
	if (this->m_sceneLoaded.load())
	{
		this->SceneUnloading();
	}

	this->m_sceneLoaded.store(false);

	if (this->m_textureCache)
	{
		this->m_textureCache->ClearCache();
	}

	this->GetGfxDevice()->WaitForIdle();
	this->GetGfxDevice()->RunGarbageCollection();

	if (this->m_loadAsync)
	{
		this->m_sceneLoadingThread = std::make_unique<std::thread>([this, fileSystem, sceneFileName]() {
				this->m_sceneLoaded.store(this->LoadScene(fileSystem, sceneFileName));
			});
	}
	else
	{
		this->m_sceneLoaded.store(this->LoadScene(fileSystem, sceneFileName));
		SceneLoaded();
	}
}
