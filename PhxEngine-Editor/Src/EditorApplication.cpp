#include <PhxEngine.h>
#include <Core/CommandLineArgs.h>
#include <Core/Containers.h>
#include <Core/StopWatch.h>
#include <Renderer/ImGuiRenderer.h>
#include <Engine/World.h>

#include <imgui.h>
#include <imgui_internal.h>

#ifdef _MSC_VER // Windows
#include <shellapi.h>
#endif 

using namespace PhxEngine;
using namespace PhxEngine::Core;

namespace
{
	class EditorApplication : public IEngineApp
	{
	public:
        EditorApplication()
        {

        }

		void Initialize() override
		{
			PhxEngine::GetTaskExecutor().silent_async([this]() {

			    this->m_isInitialize.store(true);
			});
		}

		void Finalize() override
		{
			Renderer::ImGuiRenderer::Finalize();
		}

		bool IsShuttingDown() override
		{
			return GetWindow()->ShouldClose();
		}

		void OnUpdate() override
		{
            if (!this->m_isInitialize.load())
            {
                return;
            }
		}

		void OnRender() override
		{
            Renderer::ImGuiRenderer::BeginFrame();
            if (!this->m_isInitialize.load())
            {
                return;
            }

		}

		void OnCompose(RHI::CommandList* composeCmdList) override
		{
		}

        template<typename _TWidget>
        std::shared_ptr<_TWidget> GetWidget()
        {
        }

	private:
        bool BeginWindow()
        {
			return true;
        }

    private:
      
        std::atomic_bool m_isInitialize = false;
        PhxEngine::World m_activeWorld;
	};
}

PHX_CREATE_APPLICATION(EditorApplication);
