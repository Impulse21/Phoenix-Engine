#include <PhxEngine/RHI/PhxRHI_Dx12.h>

#include "GraphicsDevice.h"

using namespace PhxEngine::RHI;
using namespace PhxEngine::RHI::Dx12;

std::unique_ptr<IGraphicsDevice> Factory::CreateDevice()
{
	return std::make_unique<GraphicsDevice>();
}
