#include "D3D12Resources.h"

#include "D3D12Common.h"

#include <PhxEngine/Core/Memory.h>
#include <RHI/D3D12/D3D12DynamicRHI.h>

#include <pix3.h>

using namespace PhxEngine::RHI;
using namespace PhxEngine::RHI::D3D12;

#if 0
void PhxEngine::RHI::D3D12::D3D12CommandList::RTBuildAccelerationStructure(RHI::RTAccelerationStructureHandle accelStructure)
{
	D3D12CommandList& internalCmd = *this->m_commandListPool.Get(cmd);
	D3D12RTAccelerationStructure* dx12AccelStructure = this->GetRTAccelerationStructurePool().Get(accelStructure);
	assert(dx12AccelStructure);


	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC dx12BuildDesc = {};
	dx12BuildDesc.Inputs = dx12AccelStructure->Dx12Desc;

	this->BuildGeometries = dx12AccelStructure->Geometries;

	// Create a local copy.
	dx12BuildDesc.Inputs.pGeometryDescs = this->BuildGeometries.data();

	switch (dx12AccelStructure->Desc.Type)
	{
	case RTAccelerationStructureDesc::Type::BottomLevel:
	{
		for (int i = 0; i < dx12AccelStructure->Desc.ButtomLevel.Geometries.size(); i++)
		{
			auto& geometry = dx12AccelStructure->Desc.ButtomLevel.Geometries[i];
			auto& buildGeometry = this->BuildGeometries[i];

			if (geometry.Flags & RTAccelerationStructureDesc::BottomLevelDesc::Geometry::kOpaque)
			{
				buildGeometry.Flags |= D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
			}
			if (geometry.Flags & RTAccelerationStructureDesc::BottomLevelDesc::Geometry::kNoduplicateAnyHitInvocation)
			{
				buildGeometry.Flags |= D3D12_RAYTRACING_GEOMETRY_FLAG_NO_DUPLICATE_ANYHIT_INVOCATION;
			}

			if (geometry.Type == RTAccelerationStructureDesc::BottomLevelDesc::Geometry::Type::Triangles)
			{
				D3D12Buffer* vertexBuffer = D3D12DynamicRHI::ResourceCast(geometry.Triangles.VertexBuffer);
				assert(vertexBuffer);
				buildGeometry.Triangles.VertexBuffer.StartAddress = vertexBuffer->D3D12Resource->GetGPUVirtualAddress() + (D3D12_GPU_VIRTUAL_ADDRESS)geometry.Triangles.VertexByteOffset;


				D3D12Buffer* indexBuffer = D3D12DynamicRHI::ResourceCast(geometry.Triangles.IndexBuffer);
				assert(indexBuffer);
				buildGeometry.Triangles.IndexBuffer = indexBuffer->D3D12Resource->GetGPUVirtualAddress() +
					(D3D12_GPU_VIRTUAL_ADDRESS)geometry.Triangles.IndexOffset * (geometry.Triangles.IndexFormat == RHI::Format::R16_UINT ? sizeof(uint16_t) : sizeof(uint32_t));

				if (geometry.Flags & RTAccelerationStructureDesc::BottomLevelDesc::Geometry::kUseTransform)
				{
					D3D12Buffer* transform3x4Buffer = D3D12DynamicRHI::ResourceCast(geometry.Triangles.Transform3x4Buffer);
					assert(transform3x4Buffer);
					buildGeometry.Triangles.Transform3x4 = transform3x4Buffer->D3D12Resource->GetGPUVirtualAddress() + (D3D12_GPU_VIRTUAL_ADDRESS)geometry.Triangles.Transform3x4BufferOffset;
				}
			}
			else if (geometry.Type == RTAccelerationStructureDesc::BottomLevelDesc::Geometry::Type::ProceduralAABB)
			{
				D3D12Buffer* aabbBuffer = D3D12DynamicRHI::ResourceCast(geometry.AABBs.AABBBuffer);
				assert(aabbBuffer);
				buildGeometry.AABBs.AABBs.StartAddress = aabbBuffer->D3D12Resource->GetGPUVirtualAddress() + (D3D12_GPU_VIRTUAL_ADDRESS)geometry.AABBs.Offset;
			}
		}
		break;
	}
	case RTAccelerationStructureDesc::Type::TopLevel:
	{
		D3D12Buffer* instanceBuffer = D3D12DynamicRHI::ResourceCast(dx12AccelStructure->Desc.TopLevel.InstanceBuffer);
		assert(instanceBuffer);

		dx12BuildDesc.Inputs.InstanceDescs = instanceBuffer->D3D12Resource->GetGPUVirtualAddress() +
			(D3D12_GPU_VIRTUAL_ADDRESS)dx12AccelStructure->Desc.TopLevel.Offset;
		break;
	}
	default:
		return;
	}

	// TODO: handle Update
	/*
	if (src != nullptr)
	{
		desc.Inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;

		auto src_internal = to_internal(src);
		desc.SourceAccelerationStructureData = src_internal->gpu_address;
	}
	*/
	dx12BuildDesc.DestAccelerationStructureData = dx12AccelStructure->D3D12Resource->GetGPUVirtualAddress();


	D3D12Buffer* scratchBuffer = D3D12DynamicRHI::ResourceCast(dx12AccelStructure->SratchBuffer);
	assert(scratchBuffer);

	dx12BuildDesc.ScratchAccelerationStructureData = scratchBuffer->D3D12Resource->GetGPUVirtualAddress();
	NativeCommandList6->BuildRaytracingAccelerationStructure(&dx12BuildDesc, 0, nullptr);
}
#endif
void PhxEngine::RHI::D3D12::D3D12CommandList::Reset()
{
	if (this->NativeCommandAllocator == nullptr)
	{
		auto& queue = D3D12DynamicRHI::GetD3D12RHI()->GetQueue(this->QueueType);
		this->NativeCommandAllocator = queue.RequestAllocator();
	}

	this->NativeCommandList->Reset(this->NativeCommandAllocator, nullptr);
}

void D3D12CommandList::BeginMarker(std::string_view name)
{
	PIXBeginEvent(NativeCommandList.Get(), 0, std::wstring(name.begin(), name.end()).c_str());
}

void D3D12CommandList::EndMarker()
{
	PIXEndEvent(NativeCommandList.Get());
}

void D3D12CommandList::TransitionBarrier(
	ITexture* texture,
	ResourceStates beforeState,
	ResourceStates afterState)
{
	D3D12Texture* textureImpl = D3D12DynamicRHI::ResourceCast(texture);
	assert(textureImpl);

	this->TransitionBarrier(
		textureImpl->D3D12Resource,
		ConvertResourceStates(beforeState),
		ConvertResourceStates(afterState));
}

void PhxEngine::RHI::D3D12::D3D12CommandList::TransitionBarrier(
	IBuffer* buffer,
	ResourceStates beforeState,
	ResourceStates afterState)
{
	D3D12Buffer* bufferImpl = D3D12DynamicRHI::ResourceCast(buffer);
	assert(bufferImpl);

	this->TransitionBarrier(
		bufferImpl->D3D12Resource,
		ConvertResourceStates(beforeState),
		ConvertResourceStates(afterState));
}

void PhxEngine::RHI::D3D12::D3D12CommandList::TransitionBarriers(Core::Span<GpuBarrier> gpuBarriers)
{
	for (const PhxEngine::RHI::GpuBarrier& gpuBarrier : gpuBarriers)
	{
		if (const GpuBarrier::TextureBarrier* texBarrier = std::get_if<GpuBarrier::TextureBarrier>(&gpuBarrier.Data))
		{
			D3D12Texture* textureImpl = D3D12DynamicRHI::ResourceCast(texBarrier->Texture);
			D3D12_RESOURCE_BARRIER barrier = {};
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier.Transition.pResource = textureImpl->D3D12Resource.Get();
			barrier.Transition.StateBefore = ConvertResourceStates(texBarrier->BeforeState);
			barrier.Transition.StateAfter = ConvertResourceStates(texBarrier->AfterState);
			barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

			if (texBarrier->Mip >= 0 || texBarrier->Slice >= 0)
			{
				barrier.Transition.Subresource = D3D12CalcSubresource(
					(UINT)std::max(0, texBarrier->Mip),
					(UINT)std::max(0, texBarrier->Slice),
					0,
					textureImpl->Desc.MipLevels,
					textureImpl->Desc.ArraySize);
			}

			this->BarrierMemoryPool.push_back(barrier);
		}
		else if (const GpuBarrier::BufferBarrier* bufferBarrier = std::get_if<GpuBarrier::BufferBarrier>(&gpuBarrier.Data))
		{
			Microsoft::WRL::ComPtr<ID3D12Resource> D3D12Resource = D3D12DynamicRHI::ResourceCast(bufferBarrier->Buffer)->D3D12Resource;
			CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
				D3D12Resource.Get(),
				ConvertResourceStates(bufferBarrier->BeforeState),
				ConvertResourceStates(bufferBarrier->AfterState));

			this->BarrierMemoryPool.push_back(barrier);
		}

		else if (const GpuBarrier::MemoryBarrier* memoryBarrier = std::get_if<GpuBarrier::MemoryBarrier>(&gpuBarrier.Data))
		{
			D3D12_RESOURCE_BARRIER barrier = {};
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier.UAV.pResource = nullptr;

			if (RHI::ITexture* const* texture = std::get_if<RHI::ITexture*>(&memoryBarrier->Resource))
			{
				D3D12Texture* textureImpl = D3D12DynamicRHI::ResourceCast(*texture);
				if (textureImpl && textureImpl->D3D12Resource != nullptr)
				{
					barrier.UAV.pResource = textureImpl->D3D12Resource.Get();
				}
			}
			else if (RHI::IBuffer* const* buffer = std::get_if<IBuffer*>(&memoryBarrier->Resource))
			{
				D3D12Buffer* bufferImpl = D3D12DynamicRHI::ResourceCast(*buffer);
				if (bufferImpl && bufferImpl->D3D12Resource != nullptr)
				{
					barrier.UAV.pResource = bufferImpl->D3D12Resource.Get();
				}
			}

			this->BarrierMemoryPool.push_back(barrier);
		}
	}

	if (!this->BarrierMemoryPool.empty())
	{
		// TODO: Batch Barrier
		NativeCommandList->ResourceBarrier(
			this->BarrierMemoryPool.size(),
			this->BarrierMemoryPool.data());

		this->BarrierMemoryPool.clear();
	}
}

void D3D12CommandList::ClearTextureFloat(ITexture* texture, Color const& clearColour)
{
	auto textureImpl = D3D12DynamicRHI::ResourceCast(texture);
	assert(textureImpl);

	assert(!textureImpl->RtvAllocation.Allocation.IsNull());
	NativeCommandList->ClearRenderTargetView(
		textureImpl->RtvAllocation.Allocation.GetCpuHandle(),
		&clearColour.R,
		0,
		nullptr);
}

#if 0
GPUAllocation D3D12CommandList::AllocateGpu(size_t bufferSize, size_t stride)
{
	assert(bufferSize <= this->UploadBuffer.GetCapacity());

	auto heapAllocation = this->UploadBuffer->Allocate(bufferSize, stride);

	GPUAllocation gpuAlloc = {};
	gpuAlloc.GpuBuffer = heapAllocation.IBuffer*;
	gpuAlloc.CpuData = heapAllocation.CpuData;
	gpuAlloc.Offset = heapAllocation.Offset;
	gpuAlloc.SizeInBytes = bufferSize;

	return gpuAlloc;
}
#endif 

void D3D12CommandList::ClearDepthStencilTexture(
	ITexture* depthStencil,
	bool clearDepth,
	float depth,
	bool clearStencil,
	uint8_t stencil)
{
	auto textureImpl = D3D12DynamicRHI::ResourceCast(depthStencil);
	assert(textureImpl);

	D3D12_CLEAR_FLAGS flags = {};
	if (clearDepth)
	{
		flags |= D3D12_CLEAR_FLAG_DEPTH;
	}

	if (clearStencil)
	{
		flags |= D3D12_CLEAR_FLAG_STENCIL;
	}

	assert(!textureImpl->DsvAllocation.Allocation.IsNull());
	NativeCommandList->ClearDepthStencilView(
		textureImpl->DsvAllocation.Allocation.GetCpuHandle(),
		flags,
		depth,
		stencil,
		0,
		nullptr);
}

void D3D12CommandList::Draw(DrawArgs const& args)
{
	NativeCommandList->DrawInstanced(
		args.VertexCount,
		args.InstanceCount,
		args.StartVertex,
		args.StartInstance);
}

void D3D12CommandList::DrawIndexed(DrawArgs const& args)
{
	NativeCommandList->DrawIndexedInstanced(
		args.IndexCount,
		args.InstanceCount,
		args.StartIndex,
		args.BaseVertex,
		args.StartInstance);
}

#if false
void PhxEngine::RHI::D3D12::D3D12CommandList::ExecuteIndirect(RHI::CommandSignatureHandle commandSignature, RHI::IBuffer* args, size_t argsOffsetInBytes, uint32_t maxCount)
{
	D3D12CommandList& internalCmd = *this->m_commandListPool.Get(cmd)
	D3D12CommandSignature* commandSignatureImpl = this->GetCommandSignaturePool().Get(commandSignature);
	D3D12Buffer* bufferImpl = D3D12DynamicRHI::ResourceCast(args);
	NativeCommandList6->ExecuteIndirect(
		commandSignatureImpl->NativeSignature.Get(),
		maxCount,
		bufferImpl->D3D12Resource.Get(),
		argsOffsetInBytes,
		nullptr,
		1);
}

void PhxEngine::RHI::D3D12::D3D12CommandList::ExecuteIndirect(
	RHI::CommandSignatureHandle commandSignature,
	RHI::IBuffer* args,
	size_t argsOffsetInBytes,
	RHI::IBuffer* count,
	size_t countOffsetInBytes,
	uint32_t maxCount)
{
	D3D12CommandSignature* commandSignatureImpl = this->GetCommandSignaturePool().Get(commandSignature);
	D3D12Buffer* argBufferImpl = D3D12DynamicRHI::ResourceCast(args);
	D3D12Buffer* countBufferBufferImpl = D3D12DynamicRHI::ResourceCast(count);
	NativeCommandList6->ExecuteIndirect(
		commandSignatureImpl->NativeSignature.Get(),
		maxCount,
		argBufferImpl->D3D12Resource.Get(),
		argsOffsetInBytes,
		countBufferBufferImpl->D3D12Resource.Get(),
		countOffsetInBytes);
}

void PhxEngine::RHI::D3D12::D3D12CommandList::DrawIndirect(RHI::IBuffer* args, size_t argsOffsetInBytes, uint32_t maxCount)
{
	D3D12Buffer* bufferImpl = D3D12DynamicRHI::ResourceCast(args);
	NativeCommandList6->ExecuteIndirect(
		this->GetDrawInstancedIndirectCommandSignature(),
		maxCount,
		bufferImpl->D3D12Resource.Get(),
		argsOffsetInBytes,
		nullptr,
		1);
}

void PhxEngine::RHI::D3D12::D3D12CommandList::DrawIndirect(RHI::IBuffer* args, size_t argsOffsetInBytes, RHI::IBuffer* count, size_t countOffsetInBytes, uint32_t maxCount)
{
	D3D12Buffer* argBufferImpl = D3D12DynamicRHI::ResourceCast(args);
	D3D12Buffer* countBufferBufferImpl = D3D12DynamicRHI::ResourceCast(args);
	NativeCommandList6->ExecuteIndirect(
		this->GetDrawInstancedIndirectCommandSignature(),
		maxCount,
		argBufferImpl->D3D12Resource.Get(),
		argsOffsetInBytes,
		countBufferBufferImpl->D3D12Resource.Get(),
		countOffsetInBytes);
}

void PhxEngine::RHI::D3D12::D3D12CommandList::DrawIndexedIndirect(RHI::IBuffer* args, size_t argsOffsetInBytes, uint32_t maxCount)
{
	D3D12Buffer* bufferImpl = D3D12DynamicRHI::ResourceCast(args);
	NativeCommandList6->ExecuteIndirect(
		this->GetDrawIndexedInstancedIndirectCommandSignature(),
		maxCount,
		bufferImpl->D3D12Resource.Get(),
		argsOffsetInBytes,
		nullptr,
		1);
}

void PhxEngine::RHI::D3D12::D3D12CommandList::DrawIndexedIndirect(RHI::IBuffer* args, size_t argsOffsetInBytes, RHI::IBuffer* count, size_t countOffsetInBytes, uint32_t maxCount)
{
	D3D12Buffer* argBufferImpl = D3D12DynamicRHI::ResourceCast(args);
	D3D12Buffer* countBufferBufferImpl = D3D12DynamicRHI::ResourceCast(args);
	NativeCommandList6->ExecuteIndirect(
		this->GetDrawIndexedInstancedIndirectCommandSignature(),
		maxCount,
		argBufferImpl->D3D12Resource.Get(),
		argsOffsetInBytes,
		countBufferBufferImpl->D3D12Resource.Get(),
		countOffsetInBytes);
}
#endif 
void D3D12CommandList::WriteBuffer(IBuffer* buffer, const void* Data, size_t dataSize, uint64_t destOffsetBytes)
{
	UINT64 alignedSize = dataSize;
	const D3D12Buffer* bufferImpl = D3D12DynamicRHI::ResourceCast(buffer);
	if ((bufferImpl->GetDesc().Binding & BindingFlags::ConstantBuffer) == BindingFlags::ConstantBuffer)
	{
		alignedSize = Core::AlignTo(alignedSize, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
	}

	auto heapAllocation = this->UploadBuffer->Allocate(dataSize, alignedSize);
	D3D12Buffer* uploadBufferImpl = D3D12DynamicRHI::ResourceCast(heapAllocation.Buffer.Get());
	memcpy(heapAllocation.CpuData, Data, dataSize);
	NativeCommandList->CopyBufferRegion(
		bufferImpl->D3D12Resource.Get(),
		destOffsetBytes,
		uploadBufferImpl->D3D12Resource.Get(),
		heapAllocation.Offset,
		dataSize);

	// TODO: See how the upload buffer can be used here
	/*
	Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource;
	ThrowIfFailed(
		this->GetD3D12Device2()->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(dataSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&intermediateResource)));

	D3D12_SUBRESOURCE_DATA subresourceData = {};
	subresourceData.pData = data;
	subresourceData.RowPitch = dataSize;
	subresourceData.SlicePitch = subresourceData.RowPitch;

	GpuIBuffer* bufferImpl = SafeCast<GpuIBuffer*>(buffer.Get());

	UpdateSubresources(
		NativeCommandList.Get(),
		bufferImpl->D3D12Resource,
		intermediateResource,
		0, 0, 1, &subresourceData);
		this->m_trackedData->NativeResources.push_back(intermediateResource);
	*/
}

void D3D12CommandList::CopyBuffer(IBuffer* dst, uint64_t dstOffset, IBuffer* src, uint64_t srcOffset, size_t sizeInBytes)
{
	const D3D12Buffer* srcBuffer = D3D12DynamicRHI::ResourceCast(src);
	const D3D12Buffer* dstBuffer = D3D12DynamicRHI::ResourceCast(dst);

	NativeCommandList->CopyBufferRegion(
		dstBuffer->D3D12Resource.Get(),
		(UINT64)dstOffset,
		srcBuffer->D3D12Resource.Get(),
		(UINT64)srcOffset,
		(UINT64)sizeInBytes);
}

void PhxEngine::RHI::D3D12::D3D12CommandList::WriteTexture(ITexture* texture, uint32_t firstSubresource, size_t numSubresources, SubresourceData* pSubresourceData)
{
	auto textureImpl = D3D12DynamicRHI::ResourceCast(texture);
	UINT64 requiredSize = GetRequiredIntermediateSize(textureImpl->D3D12Resource.Get(), firstSubresource, numSubresources);

	auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(requiredSize);
	Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource;
	ThrowIfFailed(
		D3D12DynamicRHI::GetD3D12RHI()->GetD3D12Device()->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&intermediateResource)));


	std::vector<D3D12_SUBRESOURCE_DATA> subresources(numSubresources);
	for (int i = 0; i < numSubresources; ++i)
	{
		auto& subresource = subresources[i];
		subresource.RowPitch = pSubresourceData[i].rowPitch;
		subresource.SlicePitch = pSubresourceData[i].slicePitch;
		subresource.pData = pSubresourceData[i].pData;
	}

	UpdateSubresources(
		NativeCommandList.Get(),
		textureImpl->D3D12Resource.Get(),
		intermediateResource.Get(),
		0,
		firstSubresource,
		subresources.size(),
		subresources.data());
}

void PhxEngine::RHI::D3D12::D3D12CommandList::WriteTexture(ITexture* texture, uint32_t arraySlice, uint32_t mipLevel, const void* Data, size_t rowPitch, size_t depthPitch)
{
	// LOG_CORE_FATAL("NOT IMPLEMENTED FUNCTION CALLED: CommandList::WriteTexture");
	assert(false);
	auto textureImpl = D3D12DynamicRHI::ResourceCast(texture);
	uint32_t subresource = D3D12CalcSubresource(mipLevel, arraySlice, 0, textureImpl->Desc.MipLevels, textureImpl->Desc.ArraySize);

	D3D12_RESOURCE_DESC resourceDesc = textureImpl->D3D12Resource->GetDesc();
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
	uint32_t numRows;
	uint64_t rowSizeInBytes;
	uint64_t totalBytes;
}

void PhxEngine::RHI::D3D12::D3D12CommandList::SetRenderTargets(Core::Span<ITexture*> renderTargets, ITexture* depthStencil)
{
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> renderTargetViews(renderTargets.Size());
	for (int i = 0; i < renderTargets.Size(); i++)
	{
		auto textureImpl = D3D12DynamicRHI::ResourceCast(renderTargets[i]);
		renderTargetViews[i] = textureImpl->RtvAllocation.Allocation.GetCpuHandle();
	}

	D3D12_CPU_DESCRIPTOR_HANDLE depthView = {};
	const bool hasDepth = depthStencil != nullptr;
	if (hasDepth)
	{
		auto textureImpl = D3D12DynamicRHI::ResourceCast(depthStencil);
		depthView = textureImpl->DsvAllocation.Allocation.GetCpuHandle();
	}

	NativeCommandList->OMSetRenderTargets(
		renderTargetViews.size(),
		renderTargetViews.data(),
		hasDepth,
		hasDepth ? &depthView : nullptr);
}

void PhxEngine::RHI::D3D12::D3D12CommandList::SetGfxPipeline(IGfxPipeline* graphicsPiplineHandle)
{
	this->ActivePipelineType = D3D12CommandList::D3D12CommandList::PipelineType::Gfx;
	D3D12GfxPipeline* graphisPipeline = D3D12DynamicRHI::ResourceCast(graphicsPiplineHandle);
	NativeCommandList->SetPipelineState(graphisPipeline->D3D12PipelineState.Get());

	NativeCommandList->SetGraphicsRootSignature(graphisPipeline->RootSignature.Get());

	const auto& desc = graphisPipeline->Desc;
	D3D_PRIMITIVE_TOPOLOGY topology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
	switch (desc.PrimType)
	{
	case PrimitiveType::TriangleList:
		topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		break;
	case PrimitiveType::TriangleStrip:
		topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
		break;
	case PrimitiveType::LineList:
		topology = D3D_PRIMITIVE_TOPOLOGY_LINELIST;
		break;
	default:
		assert(false);
	}
	NativeCommandList->IASetPrimitiveTopology(topology);
}

void PhxEngine::RHI::D3D12::D3D12CommandList::SetViewports(Viewport* viewports, size_t numViewports)
{
	CD3DX12_VIEWPORT dx12Viewports[16] = {};
	for (int i = 0; i < numViewports; i++)
	{
		Viewport& viewport = viewports[i];
		dx12Viewports[i] = CD3DX12_VIEWPORT(
			viewport.MinX,
			viewport.MinY,
			viewport.GetWidth(),
			viewport.GetHeight(),
			viewport.MinZ,
			viewport.MaxZ);
	}

	NativeCommandList->RSSetViewports((UINT)numViewports, dx12Viewports);
}

void PhxEngine::RHI::D3D12::D3D12CommandList::SetScissors(Rect* scissors, size_t numScissors)
{
	CD3DX12_RECT dx12Scissors[16] = {};
	for (int i = 0; i < numScissors; i++)
	{
		const Rect& scissor = scissors[i];

		dx12Scissors[i] = CD3DX12_RECT(
			scissor.MinX,
			scissor.MinY,
			scissor.MaxX,
			scissor.MaxY);
	}

	NativeCommandList->RSSetScissorRects((UINT)numScissors, dx12Scissors);
}

void PhxEngine::RHI::D3D12::D3D12CommandList::BindPushConstant(uint32_t rootParameterIndex, uint32_t sizeInBytes, const void* constants)
{
	if (this->ActivePipelineType == D3D12CommandList::PipelineType::Compute)
	{
		NativeCommandList->SetComputeRoot32BitConstants(rootParameterIndex, sizeInBytes / sizeof(uint32_t), constants, 0);
	}
	else
	{
		NativeCommandList->SetGraphicsRoot32BitConstants(rootParameterIndex, sizeInBytes / sizeof(uint32_t), constants, 0);
	}
}

void PhxEngine::RHI::D3D12::D3D12CommandList::BindConstantBuffer(size_t rootParameterIndex, IBuffer* constantBuffer)
{
	const D3D12Buffer* constantBufferImpl = D3D12DynamicRHI::ResourceCast(constantBuffer);
	if (this->ActivePipelineType == D3D12CommandList::PipelineType::Compute)
	{
		NativeCommandList->SetComputeRootConstantBufferView(rootParameterIndex, constantBufferImpl->D3D12Resource->GetGPUVirtualAddress());
	}
	else
	{
		NativeCommandList->SetGraphicsRootConstantBufferView(rootParameterIndex, constantBufferImpl->D3D12Resource->GetGPUVirtualAddress());
	}
}

void PhxEngine::RHI::D3D12::D3D12CommandList::BindDynamicConstantBuffer(size_t rootParameterIndex, size_t sizeInBytes, const void* bufferData)
{
	UploadAllocation alloc = this->UploadBuffer->Allocate(sizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
	std::memcpy(alloc.CpuData, bufferData, sizeInBytes);

	if (this->ActivePipelineType == D3D12CommandList::PipelineType::Compute)
	{
		NativeCommandList->SetComputeRootConstantBufferView(rootParameterIndex, alloc.Gpu);
	}
	else
	{
		NativeCommandList->SetGraphicsRootConstantBufferView(rootParameterIndex, alloc.Gpu);
	}
}

void PhxEngine::RHI::D3D12::D3D12CommandList::BindVertexBuffer(uint32_t slot, IBuffer* vertexBuffer)
{
	const D3D12Buffer* vertexBufferImpl = D3D12DynamicRHI::ResourceCast(vertexBuffer);

	NativeCommandList->IASetVertexBuffers(slot, 1, &vertexBufferImpl->VertexView);
}

void PhxEngine::RHI::D3D12::D3D12CommandList::BindDynamicVertexBuffer(uint32_t slot, size_t numVertices, size_t vertexSize, const void* vertexBufferData)
{
	size_t bufferSize = numVertices * vertexSize;
	auto heapAllocation = this->UploadBuffer->Allocate(bufferSize, vertexSize);
	memcpy(heapAllocation.CpuData, vertexBufferData, bufferSize);

	D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
	vertexBufferView.BufferLocation = heapAllocation.Gpu;
	vertexBufferView.SizeInBytes = static_cast<UINT>(bufferSize);
	vertexBufferView.StrideInBytes = static_cast<UINT>(vertexSize);

	NativeCommandList->IASetVertexBuffers(slot, 1, &vertexBufferView);
}

void PhxEngine::RHI::D3D12::D3D12CommandList::BindIndexBuffer(IBuffer* indexBuffer)
{
	const D3D12Buffer* bufferImpl = D3D12DynamicRHI::ResourceCast(indexBuffer);

	NativeCommandList->IASetIndexBuffer(&bufferImpl->IndexView);
}

void PhxEngine::RHI::D3D12::D3D12CommandList::BindDynamicIndexBuffer(size_t numIndicies, RHI::Format indexFormat, const void* indexBufferData)
{
	size_t indexSizeInBytes = indexFormat == RHI::Format::R16_UINT ? 2 : 4;
	size_t bufferSize = numIndicies * indexSizeInBytes;

	auto heapAllocation = this->UploadBuffer->Allocate(bufferSize, indexSizeInBytes);
	memcpy(heapAllocation.CpuData, indexBufferData, bufferSize);

	D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
	indexBufferView.BufferLocation = heapAllocation.Gpu;
	indexBufferView.SizeInBytes = static_cast<UINT>(bufferSize);
	const auto& formatMapping = GetDxgiFormatMapping(indexFormat);;

	indexBufferView.Format = formatMapping.SrvFormat;

	NativeCommandList->IASetIndexBuffer(&indexBufferView);
}

void PhxEngine::RHI::D3D12::D3D12CommandList::BindDynamicStructuredBuffer(uint32_t rootParameterIndex, size_t numElements, size_t elementSize, const void* bufferData)
{
	size_t sizeInBytes = numElements * elementSize;
	UploadAllocation alloc = this->UploadBuffer->Allocate(sizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
	std::memcpy(alloc.CpuData, bufferData, sizeInBytes);

	if (this->ActivePipelineType == D3D12CommandList::PipelineType::Compute)
	{
		NativeCommandList->SetComputeRootShaderResourceView(rootParameterIndex, alloc.Gpu);
	}
	else
	{
		NativeCommandList->SetGraphicsRootShaderResourceView(rootParameterIndex, alloc.Gpu);
	}
}

void PhxEngine::RHI::D3D12::D3D12CommandList::BindStructuredBuffer(size_t rootParameterIndex, IBuffer* buffer)
{
	const D3D12Buffer* bufferImpl = D3D12DynamicRHI::ResourceCast(buffer);

	if (this->ActivePipelineType == D3D12CommandList::PipelineType::Compute)
	{
		NativeCommandList->SetComputeRootShaderResourceView(
			rootParameterIndex,
			bufferImpl->D3D12Resource->GetGPUVirtualAddress());
	}
	else
	{
		NativeCommandList->SetGraphicsRootShaderResourceView(
			rootParameterIndex,
			bufferImpl->D3D12Resource->GetGPUVirtualAddress());
	}
}

void PhxEngine::RHI::D3D12::D3D12CommandList::BindResourceTable(size_t rootParameterIndex)
{
#if 0
	if (D3D12DynamicRHI::GetD3D12RHI()->GetMinShaderModel() < ShaderModel::SM_6_6)
	{
		if (this->ActivePipelineType == D3D12CommandList::PipelineType::Compute)
		{
			NativeCommandList->SetComputeRootDescriptorTable(
				rootParameterIndex,
				this->m_bindlessResourceDescriptorTable.GetGpuHandle(0));
		}
		else
		{
			NativeCommandList->SetGraphicsRootDescriptorTable(
				rootParameterIndex,
				this->m_bindlessResourceDescriptorTable.GetGpuHandle(0));
		}
	}
#endif
}

void PhxEngine::RHI::D3D12::D3D12CommandList::BindSamplerTable(size_t rootParameterIndex)
{
	// TODO:
	assert(false);
}
void D3D12CommandList::BindDynamicDescriptorTable(size_t rootParameterIndex, Core::Span<ITexture*> textures)
{
	// Request Descriptoprs for table
	// Validate with Root Signature. Maybe an improvment in the future.
	DescriptorHeapAllocation descriptorTable = this->ActiveDynamicSubAllocator->Allocate(textures.Size());
	for (int i = 0; i < textures.Size(); i++)
	{
		auto textureImpl = D3D12DynamicRHI::ResourceCast(textures[i]);
		D3D12DynamicRHI::GetD3D12RHI()->GetD3D12Device()->CopyDescriptorsSimple(
			1,
			descriptorTable.GetCpuHandle(i),
			textureImpl->Srv.Allocation.GetCpuHandle(),
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	if (this->ActivePipelineType == D3D12CommandList::PipelineType::Compute)
	{
		NativeCommandList->SetComputeRootDescriptorTable(rootParameterIndex, descriptorTable.GetGpuHandle());
	}
	else
	{
		NativeCommandList->SetGraphicsRootDescriptorTable(rootParameterIndex, descriptorTable.GetGpuHandle());
	}
}

void PhxEngine::RHI::D3D12::D3D12CommandList::BindDynamicUavDescriptorTable(
	size_t rootParameterIndex,
	Core::Span<IBuffer*> buffers,
	Core::Span<ITexture*> textures)
{
	// Request Descriptoprs for table
	// Validate with Root Signature. Maybe an improvment in the future.
	DescriptorHeapAllocation descriptorTable = this->ActiveDynamicSubAllocator->Allocate(buffers.Size() + textures.Size());
	for (int i = 0; i < buffers.Size(); i++)
	{
		auto impl = D3D12DynamicRHI::ResourceCast(buffers[i]);
		D3D12DynamicRHI::GetD3D12RHI()->GetD3D12Device()->CopyDescriptorsSimple(
			1,
			descriptorTable.GetCpuHandle(i),
			impl->UavAllocation.Allocation.GetCpuHandle(),
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	}

	for (int i = 0; i < textures.Size(); i++)
	{
		auto textureImpl = D3D12DynamicRHI::ResourceCast(textures[i]);
		D3D12DynamicRHI::GetD3D12RHI()->GetD3D12Device()->CopyDescriptorsSimple(
			1,
			descriptorTable.GetCpuHandle(i + buffers.Size()),
			textureImpl->UavAllocation.Allocation.GetCpuHandle(),
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	}

	if (this->ActivePipelineType == D3D12CommandList::PipelineType::Compute)
	{
		NativeCommandList->SetComputeRootDescriptorTable(rootParameterIndex, descriptorTable.GetGpuHandle());
	}
	else
	{
		NativeCommandList->SetGraphicsRootDescriptorTable(rootParameterIndex, descriptorTable.GetGpuHandle());
	}
}

void PhxEngine::RHI::D3D12::D3D12CommandList::SetComputeState(IComputePipeline* state)
{
	this->ActivePipelineType = D3D12CommandList::PipelineType::Compute;
	D3D12ComputePipeline* computePsoImpl = D3D12DynamicRHI::ResourceCast(state);
	NativeCommandList->SetComputeRootSignature(computePsoImpl->RootSignature.Get());
	NativeCommandList->SetPipelineState(computePsoImpl->D3D12PipelineState.Get());

	this->ActiveComputePipeline = computePsoImpl;
}

void PhxEngine::RHI::D3D12::D3D12CommandList::Dispatch(uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ)
{
	NativeCommandList->Dispatch(groupsX, groupsY, groupsZ);
}

void PhxEngine::RHI::D3D12::D3D12CommandList::DispatchIndirect(RHI::IBuffer* args, uint32_t argsOffsetInBytes, uint32_t maxCount)
{
#if 0
	D3D12Buffer* bufferImpl = D3D12DynamicRHI::ResourceCast(args);
	NativeCommandList6->ExecuteIndirect(
		this->GetDispatchIndirectCommandSignature(),
		maxCount,
		bufferImpl->D3D12Resource.Get(),
		argsOffsetInBytes,
		nullptr,
		1);
#endif
}

void PhxEngine::RHI::D3D12::D3D12CommandList::DispatchIndirect(RHI::IBuffer* args, uint32_t argsOffsetInBytes, RHI::IBuffer* count, size_t countOffsetInBytes, uint32_t maxCount)
{
#if 0
	D3D12Buffer* argBufferImpl = D3D12DynamicRHI::ResourceCast(args);
	D3D12Buffer* countBufferBufferImpl = D3D12DynamicRHI::ResourceCast(args);
	NativeCommandList6->ExecuteIndirect(
		this->GetDispatchIndirectCommandSignature(),
		maxCount,
		argBufferImpl->D3D12Resource.Get(),
		argsOffsetInBytes,
		countBufferBufferImpl->D3D12Resource.Get(),
		countOffsetInBytes);
#endif
}

void PhxEngine::RHI::D3D12::D3D12CommandList::SetMeshPipeline(IMeshPipeline* meshPipeline)
{
	this->ActivePipelineType = D3D12CommandList::PipelineType::Mesh;
	this->ActiveMeshPipeline = D3D12DynamicRHI::ResourceCast(meshPipeline);
	NativeCommandList->SetPipelineState(this->ActiveMeshPipeline->D3D12PipelineState.Get());

	NativeCommandList->SetGraphicsRootSignature(this->ActiveMeshPipeline->RootSignature.Get());

	const auto& desc = this->ActiveMeshPipeline->Desc;
	D3D_PRIMITIVE_TOPOLOGY topology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
	switch (desc.PrimType)
	{
	case PrimitiveType::TriangleList:
		topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		break;
	case PrimitiveType::TriangleStrip:
		topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
		break;
	case PrimitiveType::LineList:
		topology = D3D_PRIMITIVE_TOPOLOGY_LINELIST;
		break;
	default:
		assert(false);
	}
	NativeCommandList->IASetPrimitiveTopology(topology);

	// TODO: Move viewport logic here as well...
}

void PhxEngine::RHI::D3D12::D3D12CommandList::DispatchMesh(uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ)
{
	assert(this->ActivePipelineType == D3D12CommandList::PipelineType::Mesh);
	NativeCommandList6->DispatchMesh(groupsX, groupsY, groupsZ);
}

void PhxEngine::RHI::D3D12::D3D12CommandList::DispatchMeshIndirect(RHI::IBuffer* args, uint32_t argsOffsetInBytes, uint32_t maxCount)
{
#if 0
	D3D12Buffer* bufferImpl = D3D12DynamicRHI::ResourceCast(args);
	NativeCommandList6->ExecuteIndirect(
		this->GetDispatchIndirectCommandSignature(),
		maxCount,
		bufferImpl->D3D12Resource.Get(),
		argsOffsetInBytes,
		nullptr,
		1);
#endif
}

void PhxEngine::RHI::D3D12::D3D12CommandList::DispatchMeshIndirect(RHI::IBuffer* args, uint32_t argsOffsetInBytes, RHI::IBuffer* count, size_t countOffsetInBytes, uint32_t maxCount)
{
#if 0
	D3D12Buffer* argBufferImpl = D3D12DynamicRHI::ResourceCast(args);
	D3D12Buffer* countBufferBufferImpl = D3D12DynamicRHI::ResourceCast(args);
	NativeCommandList6->ExecuteIndirect(
		this->GetDispatchIndirectCommandSignature(),
		maxCount,
		argBufferImpl->D3D12Resource.Get(),
		argsOffsetInBytes,
		countBufferBufferImpl->D3D12Resource.Get(),
		countOffsetInBytes);
#endif
}
#if 0
void PhxEngine::RHI::D3D12::D3D12CommandList::BeginTimerQuery(TimerQueryHandle query)
{
	D3D12TimerQuery* queryImpl = this->GetTimerQueryPool().Get(query);

	this->TimerQueries.push_back(query);
	NativeCommandList->EndQuery(
		this->GetQueryHeap(),
		D3D12_QUERY_TYPE_TIMESTAMP,
		queryImpl->BeginQueryIndex);
}

void PhxEngine::RHI::D3D12::D3D12CommandList::EndTimerQuery(TimerQueryHandle query)
{
	D3D12TimerQuery* queryImpl = this->GetTimerQueryPool().Get(query);

	this->TimerQueries.push_back(query);

	const D3D12Buffer* timeStampQueryBuffer = D3D12DynamicRHI::ResourceCast(this->GetTimestampQueryBuffer());

	NativeCommandList->EndQuery(
		this->GetQueryHeap(),
		D3D12_QUERY_TYPE_TIMESTAMP,
		queryImpl->EndQueryIndex);

	NativeCommandList->ResolveQueryData(
		this->GetQueryHeap(),
		D3D12_QUERY_TYPE_TIMESTAMP,
		queryImpl->BeginQueryIndex,
		2,
		timeStampQueryBuffer->D3D12Resource.Get(),
		queryImpl->BeginQueryIndex * sizeof(uint64_t));
}
#endif