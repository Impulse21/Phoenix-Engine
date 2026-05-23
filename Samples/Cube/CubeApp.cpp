#include "CubeApp.h"

#include <PhxEngine/Core/Log.h>
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

void samples::CubeApp::OnFillWorld(phx::RenderWorld& /*world*/) 
{
    PHX_LOG_INFO(phx::Log::Channels::App, "Forcing World to shutdown.");
    phx::Engine::RequestExit();
}

void samples::CubeApp::OnUpdate(float /*dt*/) 
{

}

void samples::CubeApp::OnRender() 
{

}

void samples::CubeApp::OnShutdown() 
{

}
