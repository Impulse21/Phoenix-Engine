#pragma once

#include <PhxEngine/Engine/EngineRoot.h>

namespace PhxEngine
{
	namespace Graphics
	{
		class TextureCache;
	}

	namespace Renderer
	{
		class CommonPasses;
	}

	namespace Core
	{
		class IFileSystem;
	}

	class ApplicationBase : public EngineRenderPass
	{
	public:
		explicit ApplicationBase(IPhxEngineRoot* root)
			: EngineRenderPass(root)
			, m_loadAsync(false)
		{}

		void Render() override;

		virtual bool LoadScene(std::shared_ptr< Core::IFileSystem> fileSystem, std::filesystem::path sceneFilename) = 0;

		virtual void BeginLoadingScene(std::shared_ptr< Core::IFileSystem> fileSystem, const std::filesystem::path& sceneFileName);
		virtual void RenderSplashScreen() {};
		virtual void RenderScene() {};

		virtual void SceneUnloading() {};
		virtual void SceneLoaded() {};

		bool IsSceneLoading() const { return this->m_sceneLoadingThread != nullptr; }
		bool IsSceneLoaded() const { return this->m_sceneLoaded.load(); }


		std::shared_ptr<Renderer::CommonPasses> GetCommonPasses() const { return this->GetCommonPasses(); }

	protected:
		std::shared_ptr<Renderer::CommonPasses> m_commonPasses;
		std::shared_ptr<Graphics::TextureCache> m_textureCache;
		std::unique_ptr<std::thread> m_sceneLoadingThread;
		bool m_loadAsync;

	private:
		std::atomic_bool m_sceneLoaded;
		

	};
}

