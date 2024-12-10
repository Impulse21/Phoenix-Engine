#include "pch.h"
#include "phx/rhi/CommandListRecorder.h"
#include "phx/rhi/GfxDevice.h"

using namespace phx;
using namespace phx::rhi;

GfxCommandListRecorder::GfxCommandListRecorder(GfxDevice* device, CommandListHandle cmdHandle)
    : m_device(device)
{
    m_platformResource = m_device->GetCommandListPool().Get<platform::CommandListResource>(cmdHandle);
    assert(m_platformResource->Type == CommandQueueType::Graphics);

    m_platformRecorder.Open(m_platformResource);
}