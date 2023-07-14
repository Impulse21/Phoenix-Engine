#include "C:/Users/dipao/source/repos/Impulse21/Phoenix-Engine/Build/PhxEngine/CMakeFiles/PhxEngine.dir/Debug/cmake_pch.hxx"
#include <PhxEngine/Renderer/PhxRenderer.h>

using namespace PhxEngine::Renderer;

bool PhxEngine::Renderer::Initialize()
{
    Renderer::IRenderService::Impl = nullptr;

    return true;
}
bool PhxEngine::Renderer::Finialize()
{
    if (IRenderService::Impl)
    {
        delete IRenderService::Impl;

    }
    return true;
}
