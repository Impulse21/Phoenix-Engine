
#include <memory>
#include <PhxEngine/App/CameraController.h>
#include <PhxEngine/Core/Layer.h>
#include <PhxEngine/Renderer/Scene.h>
#include <PhxEngine/Renderer/TextureCache.h>
#include <PhxEngine/RHI/PhxRHI.h>

namespace PhxEngine::Editor
{
	struct EditorSettings
	{
		bool DisableIbl = true;
	};

	class EditorLayer : public PhxEngine::Core::Layer
	{
	public:
		EditorLayer();
		~EditorLayer() = default;

		void OnAttach() override;
		void OnUpdate(Core::TimeStep const& ts) override;
		void OnRender(RHI::TextureHandle& currentBuffer) override;

		Renderer::New::Scene& GetScene() { return this->m_scene; }

		EditorSettings& GetEditorSettings() { return this->m_editorSettings; }

	private:
		void CreatePSO();
		void LoadEditorResources();

		void DrawSceneImages();

		void CreateSceneLights();

	private:
		uint64_t m_loadFence;
		RHI::CommandListHandle m_commandList;
		std::shared_ptr<PhxEngine::Renderer::TextureCache> m_textureCache;
		std::shared_ptr<PhxEngine::Core::IFileSystem> m_fs;
		PhxEngine::Renderer::New::Scene m_scene;

		// Scene Rendering related stuff
		PhxEngine::RHI::TextureHandle m_depthBuffer;
		PhxEngine::RHI::GraphicsPSOHandle m_geometryPassPso;

		std::unique_ptr<ICameraController> m_cameraController;

		// Editor Specific Resources
		RHI::TextureHandle m_omniLightTex;

		EditorSettings m_editorSettings;
	};
}