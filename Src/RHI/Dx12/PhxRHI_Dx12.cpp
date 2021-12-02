#include <PhxEngine/RHI/PhxRHI_Dx12.h>

#include "GraphicsDevice.h"

using namespace PhxEngine::RHI;
using namespace PhxEngine::RHI::Dx12;

IGraphicsDevice* Factory::CreateDevice()
{
	return new GraphicsDevice();
}
