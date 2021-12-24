
#include <PhxEngine/Core/Log.h>

#include <PhxEngine/RHI/PhxRHI.h>
#include <PhxEngine/RHI/PhxRHI_Dx12.h>


#include "TestBedApp.h"
#include "PbrDemo.h"

using namespace PhxEngine;
using namespace PhxEngine::RHI;

std::unique_ptr<ApplicationBase> CreateTestApp()
{
    return std::make_unique<PbrDemo>();
}

int main()
{
    PhxEngine::LogSystem::Initialize();

    // Creating Device
    LOG_CORE_INFO("Creating DX12 Graphics Device");
    auto graphicsDevice = std::unique_ptr<IGraphicsDevice>(Dx12::Factory::CreateDevice());

    {
        auto app = CreateTestApp();
        app->Initialize(graphicsDevice.get());
        app->Run();
        app->Shutdown();
    }

    LOG_CORE_INFO("Shutting down");
    return 0;
}
