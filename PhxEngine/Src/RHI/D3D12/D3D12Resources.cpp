#include "D3D12Resources.h"

#include "D3D12DynamicRHI.h"

void PhxEngine::RHI::D3D12::D3D12SwapChain::DefereDeleteResources()
{
	D3D12DynamicRHI::GetD3D12RHI()->EnqueueDelete([tempBackBuffers = std::move(this->BackBuffers), tempViews = std::move(this->BackBuferViews)]() mutable
		{
			tempViews.clear();
			tempBackBuffers.clear();
		});
}
