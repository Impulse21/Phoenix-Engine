#pragma once

#include <PhxEngine/Core/Base.h>

#include <taskflow/taskflow.hpp>
#include <PhxEngine/Core/TimeStep.h>
#include <PhxEngine/Renderer/Window.h>
#include <PhxEngine/RHI/PhxRHI.h>

int main(int argc, char** argv);

namespace PhxEngine
{

    struct ApplicationCommandLineArgs
    {
        int Count = 0;
        char** Args = nullptr;

        const char* operator[](int index) const
        {
            PHX_CORE_ASSERT(index < Count);
            return Args[index];
        }
    };

    class Layer
    {
    public:
        Layer(const std::string& name = "Layer");
        virtual ~Layer() = default;

        virtual void OnAttach() {}
        virtual void OnDetach() {}
        virtual void OnUpdate(TimeStep ts) {}
        virtual void OnImGuiRender() {}
        virtual void OnEvent(Event& event) {}

        const std::string& GetName() const { return this->m_debugName; }
    protected:
        std::string m_debugName;
    };

    class LayerStack
    {
    public:
        LayerStack() = default;
        ~LayerStack();

        void PushLayer(Layer* layer);
        void PushOverlay(Layer* overlay);
        void PopLayer(Layer* layer);
        void PopOverlay(Layer* overlay);

        std::vector<Layer*>::iterator begin() { return this->m_layers.begin(); }
        std::vector<Layer*>::iterator end() { return this->m_layers.end(); }
        std::vector<Layer*>::reverse_iterator rbegin() { return this->m_layers.rbegin(); }
        std::vector<Layer*>::reverse_iterator rend() { return this->m_layers.rend(); }

        std::vector<Layer*>::const_iterator begin() const { return this->m_layers.begin(); }
        std::vector<Layer*>::const_iterator end()	const { return this->m_layers.end(); }
        std::vector<Layer*>::const_reverse_iterator rbegin() const { return this->m_layers.rbegin(); }
        std::vector<Layer*>::const_reverse_iterator rend() const { return this->m_layers.rend(); }
    private:
        std::vector<Layer*> m_layers;
        unsigned int m_layerInsertIndex = 0;
    };

    class Application 
    {
    public:
        Application();
        virtual ~Application();

        void PushLayer(Layer* layer);
        void PushOverlay(Layer* layer);
        
        void Close() { this->m_running = false; }

    private:
        void Run();

        IWindow* GetWindow() { return this->m_window.get(); }

    private:
        std::unique_ptr<IWindow> m_window;
        std::unique_ptr<RHI::GfxDevice> m_gfxDevice;

    private:
        LayerStack m_layerStack;
        bool m_running = true;
        bool m_minimized = false;
        TimeStep m_lastFrameTime = 0.0f;
        EventHandler<WindowResizeEvent> m_windowResizeHandler;

    private:
        inline static Application* s_singleton = nullptr;
        friend int ::main(int argc, char** argv);

    };


    // To be defined in CLIENT
    Application* CreateApplication(ApplicationCommandLineArgs args);
}