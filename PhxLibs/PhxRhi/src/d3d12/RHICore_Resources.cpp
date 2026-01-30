#include "PhxRhi/PhxRhi_pch.h"

#include <deque>

#include "PhxCore/EnumUtils.h"
#include "PhxCore/StringUtils.h"
#include "PhxRhi/RHICore.h"

#include "D3D12Base.h"
#include "D3D12Utils.h"
#include "D3D12Core.h"
#include "D3D12DescriptorHeaps.h"
#include "D3D12MemAlloc.h"
#include "D3D12CopyCtxManager.h"

using namespace phx;
using namespace phx::rhi;
using namespace phx::rhi::d3d12;

namespace
{
	class BindlessDescriptorTable
	{
	public:
		BindlessDescriptorTable() = default;
		void Initialize(DescriptorHeapAllocation&& allocation) { this->m_allocation = allocation; }
		BindlessDescriptorTable(const BindlessDescriptorTable&) = delete;
		BindlessDescriptorTable(BindlessDescriptorTable&&) = delete;
		BindlessDescriptorTable& operator = (const BindlessDescriptorTable&) = delete;
		BindlessDescriptorTable& operator = (BindlessDescriptorTable&&) = delete;

	public:
		DescriptorIndex Allocate() { return this->m_descriptorIndexPool.Allocate(); }
		void Free(DescriptorIndex index) { this->m_descriptorIndexPool.Release(index); }

		D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(DescriptorIndex index) const { return this->m_allocation.GetCpuHandle(index); }
		D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(DescriptorIndex index) const { return this->m_allocation.GetGpuHandle(index); }

	private:
		struct DescriptorIndexPool
		{
			// Removes the first element from the free list and returns its index
			UINT Allocate()
			{
				std::scoped_lock Guard(this->IndexMutex);

				UINT NewIndex;
				if (!IndexQueue.empty())
				{
					NewIndex = IndexQueue.front();
					IndexQueue.pop_front();
				}
				else
				{
					NewIndex = Index++;
				}
				return NewIndex;
			}

			void Release(UINT index)
			{
				std::scoped_lock Guard(this->IndexMutex);
				IndexQueue.push_back(index);
			}

			std::mutex IndexMutex;
			std::deque<DescriptorIndex> IndexQueue;
			UINT Index = 0;
		};

	private:
		DescriptorHeapAllocation m_allocation;
		DescriptorIndexPool m_descriptorIndexPool;
	};
}

namespace phx::rhi::d3d12
{
	Microsoft::WRL::ComPtr<D3D12MA::Allocator> g_d3d12MemAllocator;
	phx::PagedPool<rhi::PipelineState, PipelineState> g_pipelineStatePool;
	phx::PagedPool<rhi::Texture, Texture> g_texturePool;
	phx::PagedPool<rhi::GpuBuffer, d3d12::GpuBuffer> g_bufferPool;
}

namespace
{
	BindlessDescriptorTable m_bindlessDescritorTable;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_emptyRootSignature;
	CopyCtxManager m_copyCtxManager;
}

namespace
{
	// Forward Declares of some fuctions. These are defined at the end of the file as I don't touch them too often.
	void CopyBindlessDescriptor(DescriptorIndex index, D3D12_CPU_DESCRIPTOR_HANDLE srcHandle);

	void CreateSrv(GpuBufferDescriptor const& desc, ID3D12Resource* d3d12Resource, D3D12_CPU_DESCRIPTOR_HANDLE handle);
	void CreateUav(GpuBufferDescriptor const& desc, ID3D12Resource* d3d12Resource, D3D12_CPU_DESCRIPTOR_HANDLE handle);

	void CreateSrv(TextureDescriptor const& desc, ID3D12Resource* d3d12Resource, D3D12_CPU_DESCRIPTOR_HANDLE handle);
	void CreateRtv(TextureDescriptor const& desc, ID3D12Resource* d3d12Resource, D3D12_CPU_DESCRIPTOR_HANDLE handle);
	void CreateDsv(TextureDescriptor const& desc, ID3D12Resource* d3d12Resource, D3D12_CPU_DESCRIPTOR_HANDLE handle);
	void CreateUav(TextureDescriptor const& desc, ID3D12Resource* d3d12Resource, D3D12_CPU_DESCRIPTOR_HANDLE handle);
}

namespace phx::rhi::d3d12
{
	void InitializeResources(rhi::rhiCreateInfo const& createInfo)
	{
		g_pipelineStatePool.Initialize(createInfo.MaxNumTextures);
		g_bufferPool.Initialize(createInfo.MaxNumGpuBuffers);
		g_texturePool.Initialize(createInfo.MaxNumPipelineStates);

		D3D12MA::ALLOCATOR_DESC allocatorDesc = {};
		allocatorDesc.pDevice = g_d3d12Device.Get();
		allocatorDesc.pAdapter = g_adapter.NativeAdapter.Get();
		//allocatorDesc.PreferredBlockSize = 256 * 1024 * 1024;
		//allocatorDesc.Flags |= D3D12MA::ALLOCATOR_FLAG_ALWAYS_COMMITTED;
		allocatorDesc.Flags = (D3D12MA::ALLOCATOR_FLAGS)(D3D12MA::ALLOCATOR_FLAG_MSAA_TEXTURES_ALWAYS_COMMITTED | D3D12MA::ALLOCATOR_FLAG_DEFAULT_POOLS_NOT_ZEROED);

		ThrowIfFailed(
			D3D12MA::CreateAllocator(&allocatorDesc, &g_d3d12MemAllocator));

		m_bindlessDescritorTable.Initialize(
			g_gpuDescHeap_Resource->Allocate(NUM_BINDLESS_RESOURCES));


		m_emptyRootSignature = CreateEmptyRootSignature();
		m_copyCtxManager.Initialize();
	}

	void FinalizeResources()
	{
		g_pipelineStatePool.Finalize();
		g_texturePool.Finalize();
		g_bufferPool.Finalize();
	}

}

namespace phx::rhi
{
	TextureHandle CreateTexture(TextureDescriptor const& desc, MemInfo* initData)
	{
		TextureHandle handle = g_texturePool.Allocate();
		auto& impl = *g_texturePool.Get<d3d12::Texture>(handle);

		D3D12_CLEAR_VALUE d3d12OptimizedClearValue = {};
		d3d12OptimizedClearValue.Color[0] = desc.ClearValue.Colour.R;
		d3d12OptimizedClearValue.Color[1] = desc.ClearValue.Colour.G;
		d3d12OptimizedClearValue.Color[2] = desc.ClearValue.Colour.B;
		d3d12OptimizedClearValue.Color[3] = desc.ClearValue.Colour.A;
		d3d12OptimizedClearValue.DepthStencil.Depth = (UINT8)desc.ClearValue.DepthStencil.Depth;
		d3d12OptimizedClearValue.DepthStencil.Stencil = (UINT8)desc.ClearValue.DepthStencil.Stencil;

		auto dxgiFormatMapping = GetDxgiFormatMapping(desc.Format);
		d3d12OptimizedClearValue.Format = dxgiFormatMapping.RtvFormat;

		D3D12MA::ALLOCATION_DESC allocationDesc = {};
		allocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

		D3D12_RESOURCE_FLAGS resourceFlags = D3D12_RESOURCE_FLAG_NONE;
		if ((desc.BindingFlags & BindingFlags::DepthStencil) == BindingFlags::DepthStencil)
		{
			resourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
			if ((desc.BindingFlags & BindingFlags::ShaderResource) != BindingFlags::ShaderResource)
			{
				resourceFlags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
			}
		}

		if ((desc.BindingFlags & BindingFlags::RenderTarget) == BindingFlags::RenderTarget)
		{
			resourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		}

		if ((desc.BindingFlags & BindingFlags::UnorderedAccess) == BindingFlags::UnorderedAccess)
		{
			resourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		}

		CD3DX12_RESOURCE_DESC resourceDesc = {};

		switch (desc.Type)
		{
		case TextureType::Texture1D:
		{
			resourceDesc =
				CD3DX12_RESOURCE_DESC::Tex1D(EnumHasAnyFlags(desc.MiscFlags, ResourceMiscFlags::TypelessFormatCasting) ? dxgiFormatMapping.SrvFormat : dxgiFormatMapping.RtvFormat,
					desc.Width,
					desc.ArraySize,
					desc.MipLevels,
					resourceFlags);
			break;
		}
		case TextureType::Texture2D:
		{
			resourceDesc =
				CD3DX12_RESOURCE_DESC::Tex2D(EnumHasAnyFlags(desc.MiscFlags, ResourceMiscFlags::TypelessFormatCasting) ? dxgiFormatMapping.SrvFormat : dxgiFormatMapping.RtvFormat,
					desc.Width,
					desc.Height,
					desc.ArraySize,
					desc.MipLevels,
					1,
					0,
					resourceFlags);
			break;
		}
		case TextureType::Texture3D:
		{
			resourceDesc =
				CD3DX12_RESOURCE_DESC::Tex3D(
					EnumHasAnyFlags(desc.MiscFlags, ResourceMiscFlags::TypelessFormatCasting) ? dxgiFormatMapping.SrvFormat : dxgiFormatMapping.RtvFormat,
					desc.Width,
					desc.Height,
					desc.ArraySize,
					desc.MipLevels,
					resourceFlags);
			break;
		}
		default:
			throw std::runtime_error("Unsupported texture dimension");
		}

		const bool useClearValue =
			((desc.BindingFlags & BindingFlags::RenderTarget) == BindingFlags::RenderTarget) ||
			((desc.BindingFlags & BindingFlags::DepthStencil) == BindingFlags::DepthStencil);

		impl.MipLevels = desc.MipLevels;
		impl.ArraySize = desc.ArraySize;


		if (EnumHasAnyFlags(desc.MiscFlags, ResourceMiscFlags::Alias))
		{
			// Aliasing memory pool must not be a committed resource because that uses implicit heap which returns nullptr,
			//	thus it cannot be offsetted. This is why we create custom allocation here which will never be committed resource
			//	(since it has no resource)
			D3D12_RESOURCE_ALLOCATION_INFO allocationInfo = g_d3d12Device->GetResourceAllocationInfo(0, 1, &resourceDesc);

			allocationInfo.SizeInBytes = AlignUp(allocationInfo.SizeInBytes, (UINT64)D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);


			if (EnumHasAnyFlags(g_capabilities, DeviceCapability::AliasingGeneric))
			{
				allocationDesc.ExtraHeapFlags = D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES;
			}
			else if (EnumHasAnyFlags(desc.MiscFlags, ResourceMiscFlags::AliasBuffer))
			{
				allocationDesc.ExtraHeapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
			}
			else if (EnumHasAnyFlags(desc.MiscFlags, ResourceMiscFlags::AliasTexture_NonRtDs))
			{
				allocationDesc.ExtraHeapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES;
			}
			else if (EnumHasAnyFlags(desc.MiscFlags, ResourceMiscFlags::AliasTexture_RtDs))
			{
				allocationDesc.ExtraHeapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES;
			}

			ThrowIfFailed(
				g_d3d12MemAllocator->AllocateMemory(
					&allocationDesc,
					&allocationInfo,
					&impl.Allocation));

			ThrowIfFailed(
				g_d3d12Device->CreatePlacedResource(
					impl.Allocation->GetHeap(),
					impl.Allocation->GetOffset(),
					&resourceDesc,
					ConvertResourceStates(desc.InitialState),
					useClearValue ? &d3d12OptimizedClearValue : nullptr,
					IID_PPV_ARGS(&impl.Resource)));
		}
		else if (EnumHasAnyFlags(desc.MiscFlags, ResourceMiscFlags::Sparse))
		{
			resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE;
			g_d3d12Device->CreateReservedResource(
				&resourceDesc,
				ConvertResourceStates(desc.InitialState),
				useClearValue ? &d3d12OptimizedClearValue : nullptr,
				IID_PPV_ARGS(&impl.Resource));

			impl.SparsePageSize = D3D12_TILED_RESOURCE_TILE_SIZE_IN_BYTES;

			UINT num_tiles_for_entire_resource = 0;
			D3D12_PACKED_MIP_INFO packed_mip_info = {};
			D3D12_TILE_SHAPE tile_shape = {};
			UINT num_subresource_tilings = 0;

			g_d3d12Device->GetResourceTiling(
				impl.Resource.Get(),
				&num_tiles_for_entire_resource,
				&packed_mip_info,
				&tile_shape,
				&num_subresource_tilings,
				0,
				nullptr
			);

			SparseTextureProperties& sparse = impl.SparseProperties;
			sparse.TileWidth = tile_shape.WidthInTexels;
			sparse.TileHeight = tile_shape.HeightInTexels;
			sparse.TileDepth = tile_shape.DepthInTexels;
			sparse.TotalTileCount = num_tiles_for_entire_resource;
			sparse.PackedMipStart = packed_mip_info.NumStandardMips;
			sparse.PackedMipCount = packed_mip_info.NumPackedMips;
			sparse.PackedMipTileOffset = packed_mip_info.StartTileIndexInOverallResource;
			sparse.PackedMipTileCount = packed_mip_info.NumTilesForPackedMips;
		}
		else
		{
			if (desc.Alias.Buffer.IsValid())
			{
				// Aliasing: https://gpuopen-librariesandsdks.github.io/D3D12MemoryAllocator/html/resource_aliasing.html

				// TODO Handle Textures and Buffers
				auto& aliasBuffer = *g_bufferPool.Get<d3d12::GpuBuffer>(desc.Alias.Buffer);
				// TODO: Support aliasing
				ThrowIfFailed(
					g_d3d12MemAllocator->CreateAliasingResource(
						aliasBuffer.Allocation.Get(),
						desc.Alias.Offset,
						&resourceDesc,
						ConvertResourceStates(desc.InitialState),
						useClearValue ? &d3d12OptimizedClearValue : nullptr,
						IID_PPV_ARGS(&impl.Resource)));

			}
			else
			{
				// TODO: Support aliasing
				ThrowIfFailed(
					g_d3d12MemAllocator->CreateResource(
						&allocationDesc,
						&resourceDesc,
						ConvertResourceStates(desc.InitialState),
						useClearValue ? &d3d12OptimizedClearValue : nullptr,
						&impl.Allocation,
						IID_PPV_ARGS(&impl.Resource)));
			}
		}


		std::wstring debugName;
		StringConvert(desc.DebugName, debugName);
		impl.Resource->SetName(debugName.c_str());

		if (initData)
		{
			UINT64 totalSize = 0;
			std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> footprints(desc.ArraySize * std::max((uint16_t)1u, (desc.MipLevels)));
			std::vector<UINT> numRows(footprints.size());
			std::vector<UINT64> rowSizesInBytes((UINT64)footprints.size());

			g_d3d12Device2->GetCopyableFootprints(
				&resourceDesc,
				0,
				(UINT)footprints.size(),
				0,
				footprints.data(),
				numRows.data(),
				rowSizesInBytes.data(),
				&totalSize);


			CopyCtxManager::Ctx ctx = m_copyCtxManager.Begin(totalSize);

			for (size_t i = 0; i < footprints.size(); ++i)
			{
				D3D12_SUBRESOURCE_DATA data = {};
				data.RowPitch = initData[i].RowPitch;
				data.SlicePitch = initData[i].SlicePitch;
				data.pData = initData[i].Data;

				D3D12_MEMCPY_DEST DestData = {};
				DestData.pData = (void*)((UINT64)ctx.MappedData + footprints[i].Offset);
				DestData.RowPitch = (SIZE_T)footprints[i].Footprint.RowPitch;
				DestData.SlicePitch = (SIZE_T)footprints[i].Footprint.RowPitch * (SIZE_T)numRows[i];
				MemcpySubresource(&DestData, &data, (SIZE_T)rowSizesInBytes[i], numRows[i], footprints[i].Footprint.Depth);

				if (ctx.IsValid())
				{
					CD3DX12_TEXTURE_COPY_LOCATION Dst(impl.Resource.Get(), UINT(i));
					CD3DX12_TEXTURE_COPY_LOCATION Src(ctx.UploadBuffer.Get(), footprints[i]);
					ctx.CommandList->CopyTextureRegion(
						&Dst,
						0,
						0,
						0,
						&Src,
						nullptr
					);
				}
			}

			if (ctx.IsValid())
			{
				m_copyCtxManager.Submit(ctx);
			}
		}

		// Create views

		const uint32_t numDescriptorsRequired =
			(((desc.BindingFlags & BindingFlags::ShaderResource) == BindingFlags::ShaderResource) ? 1 : 0) +
			(((desc.BindingFlags & BindingFlags::UnorderedAccess) == BindingFlags::ShaderResource) ? 1 : 0);

		impl.DescriptorAllocation_CbvSrvUav = g_cpuDescHeap_Resource->Allocate(numDescriptorsRequired);

		if (((desc.BindingFlags & BindingFlags::ShaderResource) == BindingFlags::ShaderResource))
		{
			D3D12_CPU_DESCRIPTOR_HANDLE handle = impl.DescriptorAllocation_CbvSrvUav.GetCpuHandle(0);
			CreateSrv(desc, impl.Resource.Get(), handle);

			impl.BindlessIndex_Srv = m_bindlessDescritorTable.Allocate();
			CopyBindlessDescriptor(impl.BindlessIndex_Srv, handle);
		}

		if (((desc.BindingFlags & BindingFlags::UnorderedAccess) == BindingFlags::UnorderedAccess))
		{
			D3D12_CPU_DESCRIPTOR_HANDLE handle = impl.DescriptorAllocation_CbvSrvUav.GetCpuHandle(1);
			CreateUav(desc, impl.Resource.Get(), handle);

			impl.BindlessIndex_Uav = m_bindlessDescritorTable.Allocate();
			CopyBindlessDescriptor(impl.BindlessIndex_Uav, handle);
		}

		if ((desc.BindingFlags & BindingFlags::RenderTarget) == BindingFlags::RenderTarget)
		{
			impl.DescriptorAllocation_Rtv = g_cpuDescHeap_Rtv->Allocate(numDescriptorsRequired);
			CreateRtv(desc, impl.Resource.Get(), impl.DescriptorAllocation_Rtv.GetCpuHandle());
		}

		if ((desc.BindingFlags & BindingFlags::DepthStencil) == BindingFlags::DepthStencil)
		{
			impl.DescriptorAllocation_Dsv = g_cpuDescHeap_Dsv->Allocate(numDescriptorsRequired);
			CreateDsv(desc, impl.Resource.Get(), impl.DescriptorAllocation_Dsv.GetCpuHandle());
		}

		return handle;
	}

	PipelineStateHandle CreatePipelineState(PipelineStateDescriptor const& desc)
	{
		PipelineStateHandle handle = g_pipelineStatePool.Allocate();
		auto& pipeline = *g_pipelineStatePool.Get<d3d12::PipelineState>(handle);

		struct PSO_STREAM
		{
			struct PSO_STREAM1
			{
				CD3DX12_PIPELINE_STATE_STREAM_VS VS;
				CD3DX12_PIPELINE_STATE_STREAM_HS HS;
				CD3DX12_PIPELINE_STATE_STREAM_DS DS;
				CD3DX12_PIPELINE_STATE_STREAM_GS GS;
				CD3DX12_PIPELINE_STATE_STREAM_PS PS;
				CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER RS;
				CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL DSS;
				CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC BD;
				CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PT;
				CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT IL;
				CD3DX12_PIPELINE_STATE_STREAM_IB_STRIP_CUT_VALUE STRIP;
				CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSFormat;
				CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS Formats;
				CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_DESC SampleDesc;
				CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_MASK SampleMask;
				CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE ROOTSIG;
			} stream1 = {};

			struct PSO_STREAM2
			{
				CD3DX12_PIPELINE_STATE_STREAM_MS MS;
				CD3DX12_PIPELINE_STATE_STREAM_AS AS;
			} stream2 = {};
		} stream = {};

		// TODO: Cache reflection data in some way.
		if (desc.MS.IsValid())
		{
			stream.stream2.MS = { desc.MS.ByteCode.data(), desc.MS.ByteCode.size() };
			if (pipeline.RootSignature == nullptr)
			{
				pipeline.RootSignature = CreateRootSignature(desc.MS.ByteCode);
			}
		}
		if (desc.AS.IsValid())
		{
			stream.stream2.AS = { desc.AS.ByteCode.data(), desc.AS.ByteCode.size() };
			if (pipeline.RootSignature == nullptr)
			{
				pipeline.RootSignature = CreateRootSignature(desc.AS.ByteCode);
			}
		}
		if (desc.VS.IsValid())
		{
			stream.stream1.VS = { desc.VS.ByteCode.data(), desc.VS.ByteCode.size() };
			if (pipeline.RootSignature == nullptr)
			{
				pipeline.RootSignature = CreateRootSignature(desc.VS.ByteCode);
			}
		}
		if (desc.HS.IsValid())
		{
			stream.stream1.HS = { desc.HS.ByteCode.data(), desc.HS.ByteCode.size() };
			if (pipeline.RootSignature == nullptr)
			{
				pipeline.RootSignature = CreateRootSignature(desc.HS.ByteCode);
			}
		}
		if (desc.DS.IsValid())
		{
			stream.stream1.DS = { desc.DS.ByteCode.data(), desc.DS.ByteCode.size() };
			if (pipeline.RootSignature == nullptr)
			{
				pipeline.RootSignature = CreateRootSignature(desc.DS.ByteCode);
			}
		}
		if (desc.GS.IsValid())
		{
			stream.stream1.GS = { desc.GS.ByteCode.data(), desc.GS.ByteCode.size() };
			if (pipeline.RootSignature == nullptr)
			{
				pipeline.RootSignature = CreateRootSignature(desc.GS.ByteCode);
			}
		}
		if (desc.PS.IsValid())
		{
			stream.stream1.PS = { desc.PS.ByteCode.data(), desc.PS.ByteCode.size() };
			if (pipeline.RootSignature == nullptr)
			{
				pipeline.RootSignature = CreateRootSignature(desc.PS.ByteCode);
			}
		}

		if (pipeline.RootSignature == nullptr)
		{
			pipeline.RootSignature = m_emptyRootSignature;
		}

		stream.stream1.ROOTSIG = pipeline.RootSignature.Get();

		TranslateBlendState(desc.BlendState, stream.stream1.BD);
		TranslateDepthStencilState(desc.DepthStencilState, stream.stream1.DSS);
		TranslateRasterState(desc.RasterState, stream.stream1.RS);

		D3D12_INPUT_LAYOUT_DESC il = {};

		std::vector<D3D12_INPUT_ELEMENT_DESC> elements;
		if (!desc.VertexBufferBindings.IsEmpty())
		{
			il.NumElements = (uint32_t)desc.VertexBufferBindings.size();
			elements.resize(il.NumElements);
			for (uint32_t i = 0; i < il.NumElements; ++i)
			{
				auto& element = desc.VertexBufferBindings[i];
				D3D12_INPUT_ELEMENT_DESC& dx12Desc = elements[i];

				dx12Desc.SemanticName = element.SemanticName;
				dx12Desc.SemanticIndex = 0;


				const DxgiFormatMapping& formatMapping = GetDxgiFormatMapping(element.Format);
				dx12Desc.Format = formatMapping.SrvFormat;
				dx12Desc.InputSlot = element.InputSlot;
				dx12Desc.AlignedByteOffset = element.AlignedByteOffset;
				if (dx12Desc.AlignedByteOffset == VertexBufferBinding::sAppendAlignedElement)
				{
					dx12Desc.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
				}

				if (element.InputSlotClass == InputClassification::PerInstanceData)
				{
					dx12Desc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
					dx12Desc.InstanceDataStepRate = 1;
				}
				else
				{
					dx12Desc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
					dx12Desc.InstanceDataStepRate = 0;
				}
			}

		}
		il.pInputElementDescs = elements.data();
		stream.stream1.IL = il;

		stream.stream1.SampleMask = desc.SampleMask;

		pipeline.Topology = ConvertPrimitiveTopology(desc.PrimType, desc.PatchControlPoints);
		switch (desc.PrimType)
		{
		case PrimitiveType::PointList:
			stream.stream1.PT = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
			break;
		case PrimitiveType::LineList:
			stream.stream1.PT = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
			break;
		case PrimitiveType::TriangleList:
		case PrimitiveType::TriangleStrip:
			stream.stream1.PT = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			break;
		case PrimitiveType::PatchList:
			stream.stream1.PT = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
			break;
		}
		stream.stream1.STRIP = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;

		DXGI_FORMAT DSFormat = GetDxgiFormatMapping(desc.RenderPassInfo.DsFormat).RtvFormat;
		D3D12_RT_FORMAT_ARRAY formats = {};
		formats.NumRenderTargets = (UINT)desc.RenderPassInfo.RTFormats.size();
		for (uint32_t i = 0; i < formats.NumRenderTargets; ++i)
		{
			formats.RTFormats[i] = GetDxgiFormatMapping(desc.RenderPassInfo.RTFormats[i]).RtvFormat;
		}

		DXGI_SAMPLE_DESC sampleDesc = {};
		sampleDesc.Count = desc.RenderPassInfo.SampleCount;
		sampleDesc.Quality = 0;

		stream.stream1.DSFormat = DSFormat;
		stream.stream1.Formats = formats;
		stream.stream1.SampleDesc = sampleDesc;

		D3D12_PIPELINE_STATE_STREAM_DESC streamDesc = {};
		streamDesc.pPipelineStateSubobjectStream = &stream;
		streamDesc.SizeInBytes = sizeof(stream.stream1);

		if (phx::EnumHasAnyFlags(g_capabilities, DeviceCapability::MeshShading))
		{
			streamDesc.SizeInBytes += sizeof(stream.stream2);
		}

		ThrowIfFailed(
			g_d3d12Device2->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&pipeline.D3D12PipelineState)));

		return handle;
	}

	GpuBufferHandle CreateBuffer(GpuBufferDescriptor const& desc, MemInfo* initData)
	{
		GpuBufferHandle handle = g_bufferPool.Allocate();
		auto& buffer = *g_bufferPool.Get<d3d12::GpuBuffer>(handle);

		UINT64 alignedSize = desc.Size;
		if (EnumHasAnyFlags(desc.BindingFlags, BindingFlags::ConstantBuffer))
		{
			alignedSize = AlignUp(alignedSize, (UINT64)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
		}

		D3D12_RESOURCE_DESC resourceDesc;
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		resourceDesc.Width = alignedSize;
		resourceDesc.Height = 1;
		resourceDesc.MipLevels = 1;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.Alignment = 0;
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		if (EnumHasAnyFlags(desc.BindingFlags, BindingFlags::UnorderedAccess))
		{
			resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		}
		if (!EnumHasAnyFlags(desc.BindingFlags, BindingFlags::ShaderResource))
		{
			resourceDesc.Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
		}

		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.SampleDesc.Quality = 0;

		D3D12_RESOURCE_STATES resourceState = D3D12_RESOURCE_STATE_COMMON;

		D3D12MA::ALLOCATION_DESC allocationDesc = {};
		allocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
		if (desc.Usage == Usage::ReadBack)
		{
			allocationDesc.HeapType = D3D12_HEAP_TYPE_READBACK;
			resourceState = D3D12_RESOURCE_STATE_COPY_DEST;
			resourceDesc.Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
		}
		else if (desc.Usage == Usage::Upload)
		{
			allocationDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
			resourceState = D3D12_RESOURCE_STATE_GENERIC_READ;
		}
		else
		{
			resourceState = ConvertResourceStates(desc.InitialState);
		}

		if (EnumHasAnyFlags(desc.MiscFlags, ResourceMiscFlags::Alias))
		{
			// Aliasing memory pool must not be a committed resource because that uses implicit heap which returns nullptr,
			//	thus it cannot be offsetted. This is why we create custom allocation here which will never be committed resource
			//	(since it has no resource)
			D3D12_RESOURCE_ALLOCATION_INFO allocationInfo = g_d3d12Device->GetResourceAllocationInfo(0, 1, &resourceDesc);

			if (EnumHasAnyFlags(g_capabilities, DeviceCapability::AliasingGeneric))
			{
				allocationDesc.ExtraHeapFlags = D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES;
			}
			else if (EnumHasAnyFlags(desc.MiscFlags, ResourceMiscFlags::AliasBuffer))
			{
				allocationDesc.ExtraHeapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
			}
			else if (EnumHasAnyFlags(desc.MiscFlags, ResourceMiscFlags::AliasTexture_NonRtDs))
			{
				allocationDesc.ExtraHeapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES;
			}
			else if (EnumHasAnyFlags(desc.MiscFlags, ResourceMiscFlags::AliasTexture_RtDs))
			{
				allocationDesc.ExtraHeapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES;
			}

			ThrowIfFailed(
				g_d3d12MemAllocator->AllocateMemory(
					&allocationDesc,
					&allocationInfo,
					&buffer.Allocation));

			if (allocationDesc.ExtraHeapFlags == D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS || allocationDesc.ExtraHeapFlags == D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES)
			{
				ThrowIfFailed(
					g_d3d12Device->CreatePlacedResource(
						buffer.Allocation->GetHeap(),
						buffer.Allocation->GetOffset(),
						&resourceDesc,
						resourceState,
						nullptr,
						IID_PPV_ARGS(&buffer.Resource)));
			}
		}
		else if (EnumHasAnyFlags(desc.MiscFlags, ResourceMiscFlags::Sparse))
		{
			ThrowIfFailed(
				g_d3d12Device->CreateReservedResource(
					&resourceDesc,
					resourceState,
					nullptr,
					IID_PPV_ARGS(&buffer.Resource)));

			buffer.SparsePageSize = D3D12_TILED_RESOURCE_TILE_SIZE_IN_BYTES;
		}
		else
		{
			if (desc.Alias.Buffer.IsValid())
			{
				// Aliasing: https://gpuopen-librariesandsdks.github.io/D3D12MemoryAllocator/html/resource_aliasing.html

				auto& aliasBuffer = *g_bufferPool.Get<d3d12::GpuBuffer>(desc.Alias.Buffer);
				ThrowIfFailed(
					g_d3d12MemAllocator->CreateAliasingResource(
						aliasBuffer.Allocation.Get(),
						desc.Alias.Offset,
						&resourceDesc,
						resourceState,
						nullptr,
						IID_PPV_ARGS(&buffer.Resource)));
			}
			else
			{
				ThrowIfFailed(
					g_d3d12MemAllocator->CreateResource(
						&allocationDesc,
						&resourceDesc,
						resourceState,
						nullptr,
						&buffer.Allocation,
						IID_PPV_ARGS(&buffer.Resource)));
			}
		}

		if (buffer.Resource != nullptr)
		{
			buffer.GpuAddress = buffer.Resource->GetGPUVirtualAddress();
		}

		if (desc.Usage == Usage::ReadBack)
		{
			ThrowIfFailed(
				buffer.Resource->Map(0, nullptr, &buffer.CpuMappedAddress));
			buffer.MappedSize = static_cast<uint32_t>(desc.Size);
		}
		else if (desc.Usage == Usage::Upload)
		{
			D3D12_RANGE read_range = {};
			ThrowIfFailed(
				buffer.Resource->Map(0, &read_range, &buffer.CpuMappedAddress));
			buffer.MappedSize = static_cast<uint32_t>(desc.Size);
		}

		// Issue data copy on request:
		if (initData)
		{

			CopyCtxManager::Ctx ctx;

			void* mappedData = nullptr;
			if (desc.Usage == Usage::Upload)
			{
				mappedData = buffer.CpuMappedAddress;
			}
			else
			{
				ctx = m_copyCtxManager.Begin(desc.Size);
				mappedData = ctx.MappedData;
			}

			std::memcpy(mappedData, initData->Data, desc.Size);

			if (ctx.IsValid())
			{
				ctx.CommandList->CopyBufferRegion(
					buffer.Resource.Get(),
					0,
					ctx.UploadBuffer.Get(),
					0,
					desc.Size);

				m_copyCtxManager.Submit(ctx);
			}
		}

		const uint32_t numDescriptorsRequired =
			(EnumHasAnyFlags(desc.BindingFlags, BindingFlags::ShaderResource) ? 1 : 0) +
			(EnumHasAnyFlags(desc.BindingFlags, BindingFlags::UnorderedAccess) ? 1 : 0);

		buffer.DescriptorAllocation_CbvSrvUav = g_cpuDescHeap_Resource->Allocate(numDescriptorsRequired);

		if (EnumHasAnyFlags(desc.BindingFlags, BindingFlags::ShaderResource))
		{
			D3D12_CPU_DESCRIPTOR_HANDLE handle = buffer.DescriptorAllocation_CbvSrvUav.GetCpuHandle(0);
			CreateSrv(desc, buffer.Resource.Get(), handle);

			buffer.BindlessIndex_Srv = m_bindlessDescritorTable.Allocate();
			CopyBindlessDescriptor(buffer.BindlessIndex_Srv, handle);
		}

		if (EnumHasAnyFlags(desc.BindingFlags, BindingFlags::UnorderedAccess))
		{
			D3D12_CPU_DESCRIPTOR_HANDLE handle = buffer.DescriptorAllocation_CbvSrvUav.GetCpuHandle(1);
			CreateUav(desc, buffer.Resource.Get(), handle);

			buffer.BindlessIndex_Uav = m_bindlessDescritorTable.Allocate();
			CopyBindlessDescriptor(buffer.BindlessIndex_Uav, handle);
		}

		return handle;
	}

	void DeletePipeline(PipelineStateHandle handle)
	{
		d3d12::EnqueueDelete({
				g_frameCount,
				[=]()
				{
					g_pipelineStatePool.Free(handle);
				}
			});
	}

	void DeleteTexture(TextureHandle handle)
	{
		d3d12::EnqueueDelete({
				g_frameCount,
				[=]()
				{
					auto impl = g_texturePool.Get<d3d12::Texture>(handle);
					if (!impl)
						return;

					m_bindlessDescritorTable.Free(impl->BindlessIndex_Srv);
					m_bindlessDescritorTable.Free(impl->BindlessIndex_Uav);

					g_texturePool.Free(handle);
				}
			});
	}

	void DeleteBuffer(GpuBufferHandle /*handle*/)
	{

	}

	DescriptorIndex GetDescriptorIndex(TextureHandle handle, SubresouceType type)
	{
		auto* texture = g_texturePool.Get<d3d12::Texture>(handle);
		if (!texture)
		{
			return rhi::cInvalidDescriptorIndex;
		}

		switch (type)
		{
		case phx::rhi::SubresouceType::SRV:
			return texture->BindlessIndex_Srv;

		case phx::rhi::SubresouceType::UAV:
			return texture->BindlessIndex_Uav;

		case phx::rhi::SubresouceType::RTV:
		case phx::rhi::SubresouceType::DSV:
		default:
			return rhi::cInvalidDescriptorIndex;
		}
	}
}

namespace
{
	void CopyBindlessDescriptor(DescriptorIndex index, D3D12_CPU_DESCRIPTOR_HANDLE srcHandle)
	{
		if (index == cInvalidDescriptorIndex)
			return;

		g_d3d12Device->CopyDescriptorsSimple(
			1,
			m_bindlessDescritorTable.GetCpuHandle(index),
			srcHandle,
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	void CreateSrv(GpuBufferDescriptor const& desc, ID3D12Resource* d3d12Resource, D3D12_CPU_DESCRIPTOR_HANDLE handle)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		const UINT offset = 0;
		const UINT size = desc.Size;

		if (desc.Format == Format::UNKNOWN)
		{
			if (phx::EnumHasAnyFlags(desc.MiscFlags, ResourceMiscFlags::BufferStructured))
			{
				// This is a Structured Buffer
				const uint32_t stride = desc.Stride;
				// PHX_ASSERT(IsAligned(offset, (uint64_t)stride)); // structured buffer offset must be aligned to structure stride!
				srvDesc.Format = DXGI_FORMAT_UNKNOWN;
				srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
				srvDesc.Buffer.FirstElement = UINT(offset / stride);
				srvDesc.Buffer.NumElements = UINT(std::min(size, desc.Size - offset) / stride);
				srvDesc.Buffer.StructureByteStride = stride;
			}
			else
			{
				// This is a Raw Buffer
				PHX_ASSERT(phx::EnumHasAnyFlags(desc.MiscFlags, ResourceMiscFlags::BufferRaw), "Expected a raw buffer for this case");
				srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
				srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
				srvDesc.Buffer.FirstElement = UINT(offset / sizeof(uint32_t));
				srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
				srvDesc.Buffer.NumElements = UINT(std::min(size, desc.Size - offset) / sizeof(uint32_t));
			}
		}
		else
		{
			// This is a Typed Buffer
			auto dxgiFormatMapping = GetDxgiFormatMapping(desc.Format);
			const uint32_t stride = GetFormatStride(desc.Format);

			srvDesc.Format = dxgiFormatMapping.SrvFormat;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
			srvDesc.Buffer.FirstElement = UINT(offset / stride);
			srvDesc.Buffer.NumElements = UINT(std::min(size, desc.Size - offset) / stride);
		}

		g_d3d12Device->CreateShaderResourceView(
			d3d12Resource,
			&srvDesc,
			handle);
	}

	void CreateUav(GpuBufferDescriptor const& desc, ID3D12Resource* d3d12Resource, D3D12_CPU_DESCRIPTOR_HANDLE handle)
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.FirstElement = 0;
		const UINT offset = 0;
		const UINT size = desc.Size;

		if (desc.Format == Format::UNKNOWN)
		{
			if (phx::EnumHasAnyFlags(desc.MiscFlags, ResourceMiscFlags::BufferStructured))
			{
				// This is a Structured Buffer
				const uint32_t stride = desc.Stride;

				uavDesc.Format = DXGI_FORMAT_UNKNOWN;
				uavDesc.Buffer.FirstElement = UINT(offset / stride);
				uavDesc.Buffer.NumElements = UINT(std::min(size, desc.Size - offset) / stride);
				uavDesc.Buffer.StructureByteStride = stride;
			}
			else
			{
				// This is a Raw Buffer
				PHX_ASSERT(phx::EnumHasAnyFlags(desc.MiscFlags, ResourceMiscFlags::BufferRaw), "Expected a raw buffer for this case");
				uavDesc.Format = DXGI_FORMAT_R32_TYPELESS;
				uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
				uavDesc.Buffer.FirstElement = UINT(offset / sizeof(uint32_t));
				uavDesc.Buffer.NumElements = UINT(std::min(size, desc.Size - offset) / sizeof(uint32_t));
			}
		}
		else
		{
			// This is a Typed Buffer
			auto dxgiFormatMapping = GetDxgiFormatMapping(desc.Format);
			const uint32_t stride = GetFormatStride(desc.Format);

			uavDesc.Format = dxgiFormatMapping.SrvFormat;
			uavDesc.Buffer.FirstElement = UINT(offset / stride);
			uavDesc.Buffer.NumElements = UINT(std::min(size, desc.Size - offset) / stride);
		}

		g_d3d12Device2->CreateUnorderedAccessView(
			d3d12Resource,
			nullptr,
			&uavDesc,
			handle);
	}

	void CreateSrv(TextureDescriptor const& desc, ID3D12Resource* d3d12Resource, D3D12_CPU_DESCRIPTOR_HANDLE handle)
	{
		auto dxgiFormatMapping = GetDxgiFormatMapping(desc.Format);
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = dxgiFormatMapping.SrvFormat;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		uint32_t planeSlice = (srvDesc.Format == DXGI_FORMAT_X24_TYPELESS_G8_UINT) ? 1 : 0;

		const UINT firstMip = 0;
		const UINT mipCount = ~0u;
		const UINT firstSlice = 0;
		const UINT sliceCount = ~0u;

		switch (desc.Type)
		{
		case TextureType::Texture1D:
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
			srvDesc.Texture1D.MostDetailedMip = firstMip; // Subresource data
			srvDesc.Texture1D.MipLevels = mipCount;// Subresource data
			break;
		case TextureType::Texture1DArray:
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
			srvDesc.Texture1DArray.FirstArraySlice = firstSlice;
			srvDesc.Texture1DArray.ArraySize = sliceCount;// Subresource data
			srvDesc.Texture1DArray.MostDetailedMip = firstMip;// Subresource data
			srvDesc.Texture1DArray.MipLevels = mipCount;// Subresource data
			break;
		case TextureType::Texture2D:
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MostDetailedMip = firstMip;// Subresource data
			srvDesc.Texture2D.MipLevels = mipCount;// Subresource data
			srvDesc.Texture2D.PlaneSlice = planeSlice;
			break;
		case TextureType::Texture2DArray:
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
			srvDesc.Texture2DArray.FirstArraySlice = firstSlice;// Subresource data
			srvDesc.Texture2DArray.ArraySize = sliceCount;// Subresource data
			srvDesc.Texture2DArray.MostDetailedMip = firstMip;// Subresource data
			srvDesc.Texture2DArray.MipLevels = mipCount;// Subresource data
			srvDesc.Texture2DArray.PlaneSlice = planeSlice;
			break;
		case TextureType::TextureCube:
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
			srvDesc.TextureCube.MostDetailedMip = firstMip;// Subresource data
			srvDesc.TextureCube.MipLevels = mipCount;// Subresource data
			break;
		case TextureType::TextureCubeArray:
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
			srvDesc.TextureCubeArray.First2DArrayFace = 0;// Subresource data
			srvDesc.TextureCubeArray.NumCubes = desc.ArraySize / 6;// Subresource data
			srvDesc.TextureCubeArray.MostDetailedMip = firstMip;// Subresource data
			srvDesc.TextureCubeArray.MipLevels = mipCount;// Subresource data
			break;
		case TextureType::Texture2DMS:
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
			break;
		case TextureType::Texture2DMSArray:
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
			srvDesc.Texture2DMSArray.FirstArraySlice = firstSlice;// Subresource data
			srvDesc.Texture2DMSArray.ArraySize = sliceCount;// Subresource data
			break;
		case TextureType::Texture3D:
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
			srvDesc.Texture3D.MostDetailedMip = firstMip;// Subresource data
			srvDesc.Texture3D.MipLevels = mipCount;// Subresource data
			break;
		case TextureType::Unknown:
		default:
			throw std::runtime_error("Unsupported Enum");
		}

		g_d3d12Device->CreateShaderResourceView(
			d3d12Resource,
			&srvDesc,
			handle);
	}

	void CreateRtv(TextureDescriptor const& desc, ID3D12Resource* d3d12Resource, D3D12_CPU_DESCRIPTOR_HANDLE handle)
	{
		auto dxgiFormatMapping = GetDxgiFormatMapping(desc.Format);
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.Format = dxgiFormatMapping.RtvFormat;

		const UINT firstMip = 0;
		const UINT firstSlice = 0;
		const UINT sliceCount = ~0u;

		switch (desc.Type)
		{
		case TextureType::Texture1D:
			rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1D;
			rtvDesc.Texture1D.MipSlice = firstMip;
			break;
		case TextureType::Texture1DArray:
			rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
			rtvDesc.Texture1DArray.FirstArraySlice = firstSlice;
			rtvDesc.Texture1DArray.ArraySize = sliceCount;
			rtvDesc.Texture1DArray.MipSlice = firstMip;
			break;
		case TextureType::Texture2D:
			rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
			rtvDesc.Texture2D.MipSlice = firstMip;
			break;
		case TextureType::Texture2DArray:
		case TextureType::TextureCube:
		case TextureType::TextureCubeArray:
			rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
			rtvDesc.Texture2DArray.ArraySize = sliceCount;
			rtvDesc.Texture2DArray.FirstArraySlice = firstSlice;
			rtvDesc.Texture2DArray.MipSlice = firstMip;
			break;
		case TextureType::Texture2DMS:
			rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
			break;
		case TextureType::Texture2DMSArray:
			rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
			rtvDesc.Texture2DMSArray.FirstArraySlice = firstSlice;
			rtvDesc.Texture2DMSArray.ArraySize = sliceCount;
			break;
		case TextureType::Texture3D:
			rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
			rtvDesc.Texture3D.FirstWSlice = 0;
			rtvDesc.Texture3D.WSize = desc.ArraySize;
			rtvDesc.Texture3D.MipSlice = firstMip;
			break;
		case TextureType::Unknown:
		default:
			throw std::runtime_error("Unsupported Enum");
		}

		g_d3d12Device2->CreateRenderTargetView(
			d3d12Resource,
			&rtvDesc,
			handle);

	}

	void CreateDsv(TextureDescriptor const& desc, ID3D12Resource* d3d12Resource, D3D12_CPU_DESCRIPTOR_HANDLE handle)
	{
		auto dxgiFormatMapping = GetDxgiFormatMapping(desc.Format);
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Format = dxgiFormatMapping.RtvFormat;
		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

		const UINT firstMip = 0;
		const UINT firstSlice = 0;
		const UINT sliceCount = ~0u;

		switch (desc.Type)
		{
		case TextureType::Texture1D:
			dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1D;
			dsvDesc.Texture1D.MipSlice = firstMip;
			break;
		case TextureType::Texture1DArray:
			dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1DARRAY;
			dsvDesc.Texture1DArray.FirstArraySlice = firstSlice;
			dsvDesc.Texture1DArray.ArraySize = sliceCount;
			dsvDesc.Texture1DArray.MipSlice = firstMip;
			break;
		case TextureType::Texture2D:
			dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			dsvDesc.Texture2D.MipSlice = firstMip;
			break;
		case TextureType::Texture2DArray:
		case TextureType::TextureCube:
		case TextureType::TextureCubeArray:
			dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
			dsvDesc.Texture2DArray.ArraySize = sliceCount;
			dsvDesc.Texture2DArray.FirstArraySlice = firstSlice;
			dsvDesc.Texture2DArray.MipSlice = firstMip;
			break;
		case TextureType::Texture2DMS:
			dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
			break;
		case TextureType::Texture2DMSArray:
			dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
			dsvDesc.Texture2DMSArray.FirstArraySlice = firstSlice;
			dsvDesc.Texture2DMSArray.ArraySize = sliceCount;
			break;
		case TextureType::Texture3D:
		{
			throw std::runtime_error("Unsupported Dimension");
		}
		case TextureType::Unknown:
		default:
			throw std::runtime_error("Unsupported Enum");
		}

		g_d3d12Device2->CreateDepthStencilView(
			d3d12Resource,
			&dsvDesc,
			handle);
	}

	void CreateUav(TextureDescriptor const& desc, ID3D12Resource* d3d12Resource, D3D12_CPU_DESCRIPTOR_HANDLE handle)
	{
		auto dxgiFormatMapping = GetDxgiFormatMapping(desc.Format);
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = dxgiFormatMapping.SrvFormat;

		const UINT firstMip = 0;
		const UINT firstSlice = 0;
		const UINT sliceCount = ~0u;

		switch (desc.Type)
		{
		case TextureType::Texture1D:
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
			uavDesc.Texture1D.MipSlice = firstMip;
			break;
		case TextureType::Texture1DArray:
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
			uavDesc.Texture1DArray.FirstArraySlice = firstSlice;
			uavDesc.Texture1DArray.ArraySize = sliceCount;
			uavDesc.Texture1DArray.MipSlice = firstMip;
			break;
		case TextureType::Texture2D:
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
			uavDesc.Texture2D.MipSlice = firstMip;
			break;
		case TextureType::Texture2DArray:
		case TextureType::TextureCube:
		case TextureType::TextureCubeArray:
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
			uavDesc.Texture2DArray.FirstArraySlice = firstSlice;
			uavDesc.Texture2DArray.ArraySize = sliceCount;
			uavDesc.Texture2DArray.MipSlice = firstMip;
			break;
		case TextureType::Texture3D:
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
			uavDesc.Texture3D.FirstWSlice = 0;
			uavDesc.Texture3D.WSize = desc.Depth;
			uavDesc.Texture3D.MipSlice = firstMip;
			break;
		case TextureType::Texture2DMS:
		case TextureType::Texture2DMSArray:
		{
			throw std::runtime_error("Unsupported Dimension");
		}
		case TextureType::Unknown:
		default:
			throw std::runtime_error("Unsupported Enum");
		}

		g_d3d12Device2->CreateUnorderedAccessView(
			d3d12Resource,
			nullptr,
			&uavDesc,
			handle);
	}
}
