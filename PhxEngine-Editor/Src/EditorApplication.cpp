#include <PhxEngine/PhxEngine.h>

#include <PhxEngine/Engine/ApplicationBase.h>
#include <imgui.h>

#include <conio.h>

#include <iostream>
#include <PhxEngine/Engine/ImguiRenderer.h>
#include <PhxEngine/Core/Window.h>

using namespace PhxEngine;
using namespace PhxEngine::Core;
using namespace PhxEngine::RHI;
using namespace PhxEngine::Graphics;
using namespace PhxEngine::Renderer;

class PhxEngineEditorApp : public IEngineApplication
{
private:

public:
    PhxEngineEditorApp() = default;

    bool Initialize() override
    {
        this->m_profile = {};

        LOG_CORE_INFO("Initializing Editor App");
        WindowSpecification windowSpec = {};
        windowSpec.Width = 1280;
        windowSpec.Height = 720;
        windowSpec.Title = "Phx Engine Editor";
        windowSpec.VSync = false;

        this->m_window = WindowFactory::CreateGlfwWindow(windowSpec);
        this->m_window->Initialize();
        this->m_window->SetResizeable(false);
        this->m_window->SetVSync(windowSpec.VSync);
        this->m_window->SetEventCallback(
            [this](Event& e) {
                this->ProcessEvent(e);
            });

        this->m_windowVisibile = true;

       GfxDevice::Impl->CreateViewport(
            {
                .WindowHandle = static_cast<Platform::WindowHandle>(m_window->GetNativeWindowHandle()),
                .Width = this->m_window->GetWidth(),
                .Height = this->m_window->GetHeight(),
                .Format = RHI::RHIFormat::R10G10B10A2_UNORM,
            });

       DirectX::XMFLOAT2 m_canvasSize;

       return true;
    }

    bool Finalize() override
    {
        LOG_CORE_INFO("PhxEngine is Finalizing.");
        return true;
    }

    bool ShuttingDown() const { return this->m_shuttingDown; }

    bool RunFrame()
    {
        TimeStep deltaTime = this->m_frameTimer.Elapsed();
        this->m_frameTimer.Begin();

        // polls events.
        this->m_window->OnUpdate();

        if (this->m_windowVisibile)
        {
            // Run Sub-System updates

            // auto updateLoopId = this->m_frameProfiler->BeginRangeCPU("Update Loop");
            // this->Update(deltaTime);
            //this->m_frameProfiler->EndRangeCPU(updateLoopId);

            // auto renderLoopId = this->m_frameProfiler->BeginRangeCPU("Render Loop");
            // this->Render();
            // this->m_frameProfiler->EndRangeCPU(renderLoopId);
        }

        this->m_profile.UpdateAverageFrameTime(deltaTime);
        this->SetInformativeWindowTitle("Phx Editor", {});
        return true;
    }

private:
    void SetInformativeWindowTitle(std::string_view appName, std::string_view extraInfo)
    {
        std::stringstream ss;
        ss << appName;
        ss << "(D3D12)";

        double frameTime = this->m_profile.AverageFrameTime;
        if (frameTime > 0)
        {
            // ss << "=>" << std::setprecision(4) << frameTime << " CPU (ms) - " << std::setprecision(4) << (1.0f / frameTime) << " FPS";
            ss << "- " << std::setprecision(4) << (1.0f / frameTime) << " FPS";
        }
        const RHI::MemoryUsage memoryUsage = GfxDevice::Impl->GetMemoryUsage();

        ss << " [ " << PhxToMB(memoryUsage.Usage) << "/" << PhxToMB(memoryUsage.Budget) << " (MB) ]";
        if (!extraInfo.empty())
        {
            ss << extraInfo;
        }

        this->m_window->SetWindowTitle(ss.str());
    }

    void ProcessEvent(Event& e)
    {
        if (e.GetEventType() == WindowCloseEvent::GetStaticType())
        {
            this->m_shuttingDown = true;
            e.IsHandled = true;

            return;
        }

        if (e.GetEventType() == WindowResizeEvent::GetStaticType())
        {
            WindowResizeEvent& resizeEvent = static_cast<WindowResizeEvent&>(e);
            if (resizeEvent.GetWidth() == 0 && resizeEvent.GetHeight() == 0)
            {
                this->m_windowVisibile = true;
            }
            else
            {
                this->m_windowVisibile = false;
                // Notify GFX to resize!

                // Trigger Resize event on RHI;
                /*
                for (auto& pass : this->m_renderPasses)
                {
                    pass->OnWindowResize(resizeEvent);
                }
                */
            }
        }
    }
    private:
        class SimpleProfiler
        {
        public:
            double AverageFrameTime = 0.0f;

            void UpdateAverageFrameTime(double elapsedTime)
            {
                this->m_frameTimeSum += elapsedTime;
                this->m_numAccumulatedFrames += 1;

                if (this->m_frameTimeSum > this->m_averageTimeUpdateInterval && this->m_numAccumulatedFrames > 0)
                {
                    this->AverageFrameTime = m_frameTimeSum / double(this->m_numAccumulatedFrames);
                    this->m_numAccumulatedFrames = 0;
                    this->m_frameTimeSum = 0.0;
                }
            }

        private:
            double m_frameTimeSum = 0.0f;
            uint64_t m_numAccumulatedFrames = 0;
            double m_averageTimeUpdateInterval = 0.5f;
        } m_profile = {};
private:
    std::unique_ptr<IWindow> m_window;

    bool m_windowVisibile = false;
    bool m_shuttingDown = false;
    Core::StopWatch m_frameTimer; // TODO: Move to a profiler and scope
};


class PhxEngineEditorUI : public PhxEngine::ImGuiRenderer
{
private:

public:
    PhxEngineEditorUI(PhxEngineEditorApp* app)
        : m_app(app)
    {
    }

    void BuildUI() override
    {
    }

private:
    PhxEngineEditorApp* m_app;
};


#if 0
#ifdef WIN32
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#else
int main(int __argc, const char** __argv)
#endif
#endif 

int main(int __argc, const char** __argv)
{
    EngineParam params = {};
    params.Name = "Phx Editor";
    params.GraphicsAPI = RHI::GraphicsAPI::DX12;
    params.WindowWidth = 2000;
    params.WindowHeight = 1200;
    params.MaximumDynamicSize = PhxGB(1);


    PhxEngine::EngineInitialize(params);

    {
        PhxEngineEditorApp engineApp;
        PhxEngine::EngineRun(engineApp);
    }

    PhxEngine::EngineFinalize();

    return 0;
}