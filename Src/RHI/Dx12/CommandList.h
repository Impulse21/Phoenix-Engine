#pragma once

#include <PhxEngine/RHI/PhxRHI.h>
#include "Common.h"
#include "CommandAllocatorPool.h"

namespace PhxEngine::RHI::Dx12
{
	class GraphicsDevice;
	class CommandQueue;

	struct TrackedResources
	{
		std::vector<RefCountPtr<IResource>> Resource;
		std::vector<RefCountPtr<IUnknown>> NativeResources;
	};

	class CommandList : public RefCounter<ICommandList>
	{
	public:
		CommandList(
			GraphicsDevice& graphicsDevice,
			CommandListDesc const& desc);

		// -- Interface implementations ---
	public:
		void Open() override;
		void Close() override;

		ScopedMarker BeginScropedMarker(std::string name) override;
		void BeginMarker(std::string name) override;
		void EndMarker() override;

		void TransitionBarrier(ITexture* texture, ResourceStates beforeState, ResourceStates afterState) override;
		void TransitionBarrier(IBuffer* buffer, ResourceStates beforeState, ResourceStates afterState) override;

		void ClearTextureFloat(ITexture* texture, Color const& clearColour) override;
		void ClearDepthStencilTexture(ITexture* depthStencil, bool clearDepth, float depth, bool clearStencil, uint8_t stencil) override;

		void WriteTexture(TextureHandle texture, uint32_t firstSubResource, size_t numSubResources, SubresourceData* pSubResourceData) override;

	public:
		std::shared_ptr<TrackedResources> Executed(uint64_t fenceValue);
		ID3D12CommandList* GetD3D12CommandList() { return this->m_d3d12CommandList; }

	private:
		void TransitionBarrier(RefCountPtr<ID3D12Resource> d3d12Resource, D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState);

	private:
		GraphicsDevice& m_graphicsDevice;
		CommandListDesc m_desc = {};
		CommandAllocatorPool m_commandAlloatorPool;

		std::shared_ptr<TrackedResources> m_trackedData;
		RefCountPtr<ID3D12CommandAllocator> m_activeD3D12CommandAllocator;
		RefCountPtr<ID3D12GraphicsCommandList> m_d3d12CommandList;
		RefCountPtr<ID3D12GraphicsCommandList4> m_d3d12CommnadList4;

	};
}

