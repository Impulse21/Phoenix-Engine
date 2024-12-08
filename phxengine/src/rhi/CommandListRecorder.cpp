#include "pch.h"
#include "phx/rhi/CommandListRecorder.h"
#include "phx/rhi/GfxDevice.h"

using namespace phx;
using namespace phx::rhi;

GfxCommandListRecorder::GfxCommandListRecorder(GfxDevice* device, platform::CommandContextResource* context)
    : m_device(device)
    , m_platformContext(context)
{
}

void GfxCommandListRecorder::Finished()
{

}