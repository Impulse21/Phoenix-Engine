#include "C:/Users/dipao/source/repos/Impulse21/Phoenix-Engine/Build/PhxEngine/CMakeFiles/PhxEngine.dir/Debug/cmake_pch.hxx"

#include "RenderFactory.h"
#include <PhxEngine/Core/Helpers.h>

using namespace PhxEngine::Core;
using namespace PhxEngine::Renderer;

constexpr uint64_t kVertexBufferAlignment = 16ull;

RenderResourceFactory::RenderResourceFactory(RHI::IGraphicsDevice* gfxDevice, Core::IAllocator* allocator)
	: m_renderMeshes(allocator, 16)
	, m_allocator(allocator)
	, m_gfxDevice(m_gfxDevice)
{
}

void PhxEngine::Renderer::RenderResourceFactory::Initialize()
{
}

void PhxEngine::Renderer::RenderResourceFactory::Finialize()
{
}

RenderMeshHandle PhxEngine::Renderer::RenderResourceFactory::CreateRenderMesh(RenderMeshDesc const& desc)
{
	// Create render resources
	Core::Handle<RenderMesh> retVal = this->m_renderMeshes.Emplace();
	InternalRenderMesh& renderMesh = *this->m_renderMeshes.Get(retVal);

	renderMesh.IndexBuffer = this->m_gfxDevice->CreateIndexBuffer({
			.StrideInBytes = sizeof(uint32_t),
			.SizeInBytes = sizeof(uint32_t) * desc.Indices.Size(),
			.DebugName = "Index Buffer" });
	renderMesh.IndexBufferDescIndex = this->m_gfxDevice->GetDescriptorIndex(renderMesh.IndexBuffer, RHI::SubresouceType::SRV);

	uint64_t vertexBufferSize =
		Helpers::AlignTo(desc.Position.Size() * sizeof(DirectX::XMFLOAT3), kVertexBufferAlignment) +
		Helpers::AlignTo(desc.TexCoords.Size() * sizeof(DirectX::XMFLOAT2), kVertexBufferAlignment) +
		Helpers::AlignTo(desc.Normals.Size() * sizeof(DirectX::XMFLOAT3), kVertexBufferAlignment) +
		Helpers::AlignTo(desc.Tangents.Size() * sizeof(DirectX::XMFLOAT4), kVertexBufferAlignment) +
		Helpers::AlignTo(desc.Colour.Size() * sizeof(DirectX::XMFLOAT3), kVertexBufferAlignment);

	renderMesh.VertexBuffer = this->m_gfxDevice->CreateIndexBuffer({
			.MiscFlags = RHI::BufferMiscFlags::Raw | RHI::BufferMiscFlags::Bindless,
			.Binding = RHI::BindingFlags::VertexBuffer | RHI::BindingFlags::ShaderResource,
			.StrideInBytes = sizeof(float),
			.SizeInBytes = vertexBufferSize,
			.DebugName = "Vertex Buffer" });
	ResourceUpload vertexUploadBuffer = CreateResourceUpload(vertexBufferSize);

	// FILL Data
	RHI::ICommandList* commandList = this->m_gfxDevice->BeginCommandRecording();
	{
		RHI::GpuBarrier preBarriers[] =
		{
			RHI::GpuBarrier::CreateBuffer(renderMesh.IndexBuffer, RHI::ResourceStates::Common, RHI::ResourceStates::CopyDest),
			RHI::GpuBarrier::CreateBuffer(renderMesh.VertexBuffer, RHI::ResourceStates::Common, RHI::ResourceStates::CopyDest),
		};
		commandList->TransitionBarriers(Core::Span<RHI::GpuBarrier>(preBarriers, _countof(preBarriers)));

	}

	commandList->WriteBuffer(
		renderMesh.IndexBuffer,
		desc.Indices.begin(),
		desc.Indices.Size() * sizeof(uint32_t));

	uint64_t bufferOffset = 0ull;
	auto WriteVertexData = [&](InternalRenderMesh::VertexAttribute attr, void* data, uint64_t sizeInBytes)
	{
		auto& bufferRange = renderMesh.GetVertexAttribute(attr);
		bufferRange.ByteOffset = bufferOffset;
		bufferRange.SizeInBytes = sizeInBytes;
		bufferOffset += Helpers::AlignTo(bufferRange.SizeInBytes, kVertexBufferAlignment);
		vertexUploadBuffer.SetData(data, bufferRange.SizeInBytes, kVertexBufferAlignment);
	};

	commandList->CopyBuffer(
		renderMesh.VertexBuffer,
		0,
		vertexUploadBuffer.UploadBuffer,
		vertexUploadBuffer.Offset,
		vertexBufferSize);

	{
		RHI::GpuBarrier postBarriers[] =
		{
			RHI::GpuBarrier::CreateBuffer(renderMesh.IndexBuffer, RHI::ResourceStates::CopyDest, RHI::ResourceStates::IndexGpuBuffer | RHI::ResourceStates::ShaderResource),
			RHI::GpuBarrier::CreateBuffer(renderMesh.VertexBuffer, RHI::ResourceStates::CopyDest, RHI::ResourceStates::ShaderResource),
		};
		commandList->TransitionBarriers(Core::Span<RHI::GpuBarrier>(postBarriers, _countof(postBarriers)));
	}

	this->m_gfxDevice->ExecuteCommandLists({ commandList });
	vertexUploadBuffer.Free();

	return retVal;
}

