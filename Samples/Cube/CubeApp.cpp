#include "CubeApp.h"

#include <PhxEngine/Platform/EntryPoint.h>

using namespace samples;

PHX_DEFINE_APP(CubeApp);

const phx::EngineDesc& samples::CubeApp::GetDesc()
{
  static phx::EngineDesc desc = {.app_name = "PhxCubeApp",
                                 .window_width = 1280,
                                 .window_height = 720,
                                 .headless = false};

  return desc;
}
