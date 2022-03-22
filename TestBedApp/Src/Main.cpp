
#include <PhxEngine/PhxEngine.h>

#include <PhxEngine/RHI/PhxRHI_Dx12.h>

#include "PbrDemo.h"

using namespace PhxEngine;
using namespace PhxEngine::RHI;

std::unique_ptr<ApplicationBase> CreateTestApp()
{
    return std::make_unique<PbrDemo>();
}

int main()
{
    // Initialize Engine
    PhxEngine::LogSystem::Initialize();

    // Creating Device
    LOG_CORE_INFO("Creating DX12 Graphics Device");
    auto graphicsDevice = Dx12::Factory::CreateDevice();

    {
        EngineEnvironment e = {};
        e.pGraphicsDevice = graphicsDevice.get();
        e.pRenderSystem = nullptr;
        auto app = CreateTestApp();
        app->Initialize(&e);
        app->RunFrame();
        app->Shutdown();
    }

    LOG_CORE_INFO("Shutting down");
    return 0;
}
