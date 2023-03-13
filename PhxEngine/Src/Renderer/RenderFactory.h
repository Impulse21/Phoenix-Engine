#pragma once

#include <PhxEngine/Renderer/Renderer.h>
#include <PhxEngine/Core/Pool.h>

namespace PhxEngine::Renderer
{
	struct InternalRenderMesh
	{
		RHI::BufferHandle IndexBuffer;
		RHI::DescriptorIndex IndexBufferDescIndex;

		RHI::BufferHandle VertexBuffer;
		RHI::DescriptorIndex VertexBufferDescIndex;
	};

	class RenderResourceFactory : public IRenderResourceFactory
	{
	public:
		RenderResourceFactory(RHI::IGraphicsDevice* gfxDevice, Core::IAllocator* allocator);
		~RenderResourceFactory() = default;

		void Initialize() override;
		void Finialize() override;

		RenderMeshHandle CreateRenderMesh(RenderMeshDesc const& desc) override;
		void DeleteRenderMesh(RenderMeshHandle handle) override;

	private:
		Core::IAllocator* m_allocator;
		RHI::IGraphicsDevice* m_gfxDevice;
		Core::Pool<InternalRenderMesh, RenderMesh> m_renderMeshes;

	};
}

