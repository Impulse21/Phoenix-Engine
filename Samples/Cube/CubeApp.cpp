#include "CubeApp.h"

#include <PhxEngine/Core/PhxDefines.h>
#include <PhxEngine/Platform/EntryPoint.h>
#include <PhxEngine/Engine.h>

using namespace samples;

PHX_DEFINE_APP(CubeApp);

const phx::EngineDesc& samples::CubeApp::GetEngineDesc()
{
  static phx::EngineDesc s_desc = {.app_name = "PhxCubeApp",
                                 .window_width = 1280,
                                 .window_height = 720,
                                 .headless = false};

  return s_desc;
}

void samples::CubeApp::OnInit() 
{

}

void samples::CubeApp::OnCache(phx::RenderWorld& /*world*/) 
{
    phx::Engine::RequestExit();
}

void samples::CubeApp::OnUpdate(float /*dt*/) 
{

}

void samples::CubeApp::OnShutdown() 
{

}
