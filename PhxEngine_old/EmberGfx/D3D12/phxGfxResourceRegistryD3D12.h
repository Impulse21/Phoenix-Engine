#pragma once

#include "phxGfxDescriptorHeapsD3D12.h"
#include "EmberGfx/phxGfxDeviceResources.h"
#include "d3d12ma/D3D12MemAlloc.h"

#include "EmberGfx/phxHandle.h"
#include "EmberGfx/phxHandlePool.h"

namespace phx::gfx
{
	struct D3D12GfxPipeline final
	{
		Microsoft::WRL::ComPtr<ID3D12PipelineState> D3D12PipelineState;
		Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSignature;
	};

	struct DescriptorView final
	{
		DescriptorHeapAllocation Allocation;
		DescriptorIndex BindlessIndex = cInvalidDescriptorIndex;
		D3D12_DESCRIPTOR_HEAP_TYPE Type = {};
		union
		{
			D3D12_CONSTANT_BUFFER_VIEW_DESC CBVDesc;
			D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;
			D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc;
			D3D12_SAMPLER_DESC SAMDesc;
			D3D12_RENDER_TARGET_VIEW_DESC RTVDesc;
			D3D12_DEPTH_STENCIL_VIEW_DESC DSVDesc;
		};

		uint32_t FirstMip = 0;
		uint32_t MipCount = 0;
		uint32_t FirstSlice = 0;
		uint32_t SliceCount = 0;
	};

	struct D3D12Texture final
	{
		Microsoft::WRL::ComPtr<ID3D12Resource> D3D12Resource;
		Microsoft::WRL::ComPtr<D3D12MA::Allocation> Allocation;
		// -- The views ---
		DescriptorView RtvAllocation;
		std::vector<DescriptorView> RtvSubresourcesAlloc = {};

		DescriptorView DsvAllocation;
		std::vector<DescriptorView> DsvSubresourcesAlloc = {};

		DescriptorView Srv;
		std::vector<DescriptorView> SrvSubresourcesAlloc = {};

		DescriptorView UavAllocation;
		std::vector<DescriptorView> UavSubresourcesAlloc = {};

		uint16_t MipLevels;
		uint16_t ArraySize;

		void DisposeViews()
		{
			RtvAllocation.Allocation.Free();
			for (auto& view : RtvSubresourcesAlloc)
			{
				view.Allocation.Free();
			}
			RtvSubresourcesAlloc.clear();
			RtvAllocation = {};

			DsvAllocation.Allocation.Free();
			for (auto& view : DsvSubresourcesAlloc)
			{
				view.Allocation.Free();
			}
			DsvSubresourcesAlloc.clear();
			DsvAllocation = {};

			Srv.Allocation.Free();
			for (auto& view : SrvSubresourcesAlloc)
			{
				view.Allocation.Free();
			}
			SrvSubresourcesAlloc.clear();
			Srv = {};

			UavAllocation.Allocation.Free();
			for (auto& view : UavSubresourcesAlloc)
			{
				view.Allocation.Free();
			}
			UavSubresourcesAlloc.clear();
			UavAllocation = {};
		}
	};

	struct D3D12Buffer final
	{
		Microsoft::WRL::ComPtr<ID3D12Resource> D3D12Resource;
		Microsoft::WRL::ComPtr<D3D12MA::Allocation> Allocation;

		void* MappedData = nullptr;
		uint32_t MappedSizeInBytes = 0;

		// -- Views ---
		DescriptorView Srv;
		std::vector<DescriptorView> SrvSubresourcesAlloc = {};

		DescriptorView UavAllocation;
		std::vector<DescriptorView> UavSubresourcesAlloc = {};

		D3D12_VERTEX_BUFFER_VIEW VertexView = {};
		D3D12_INDEX_BUFFER_VIEW IndexView = {};

		void DisposeViews()
		{
			Srv.Allocation.Free();
			for (auto& view : SrvSubresourcesAlloc)
			{
				view.Allocation.Free();
			}
			SrvSubresourcesAlloc.clear();
			Srv = {};

			UavAllocation.Allocation.Free();
			for (auto& view : UavSubresourcesAlloc)
			{
				view.Allocation.Free();
			}
			UavSubresourcesAlloc.clear();
			UavAllocation = {};
		}
	};

	struct D3D12InputLayout
	{
		std::vector<VertexAttributeDesc> Attributes;
		std::vector<D3D12_INPUT_ELEMENT_DESC> InputElements;
	};



	using HandlePoolInputLayout = HandlePool<D3D12InputLayout, InputLayout>;
	using HandlePoolGfxPipeline = HandlePool<D3D12GfxPipeline, GfxPipeline>;
	using HandlePoolTexture = HandlePool<D3D12Texture, Texture>;
	using HandlePoolBuffer = HandlePool<D3D12Buffer, Buffer>;

	struct ResourceRegistryD3D12
	{
		HandlePoolInputLayout InputLayouts;
		HandlePoolGfxPipeline GfxPipelines;
		HandlePoolTexture Textures;
		HandlePoolBuffer Buffers;

		void Initialize()
		{
			this->InputLayouts.Initialize();
			this->GfxPipelines.Initialize();
			this->Textures.Initialize();
			this->Buffers.Initialize();
		}

		void Finalize()
		{
			this->InputLayouts.Finalize();
			this->GfxPipelines.Finalize();
			this->Textures.Finalize();
			this->Buffers.Finalize();
		}
	};
}

