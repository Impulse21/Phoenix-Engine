
#include <PhxEngine/RHI/PhxRHIResources.h>
#include <PhxEngine/RHI/PhxRHI.h>

using namespace PhxEngine::RHI;

void PhxEngine::RHI::RHIResource::Destroy()
{
	DynamicRHI* rhi = RHI::GetDynamic();
	if (rhi)
	{
		RHI::GetDynamic()->EnqueueDelete([this]
			{
				phx_delete(this);
			});
	}
	else
	{
		phx_delete(this);
	}
}
