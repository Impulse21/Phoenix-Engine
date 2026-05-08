#pragma once

namespace phx
{
    class IApplication;;
    namespace Engine
    {
        void Initialize(IApplication& app);
        void Run();
        void Shutdown();
    }
}