#pragma once

#include <PhxEngine/Renderer/Renderer.h>
#include <PhxEngine/Core/Pool.h>
#include <PhxEngine/Core/Primitives.h>

namespace PhxEngine::Renderer
{
	struct InternalRenderMesh
	{
		RHI::BufferHandle IndexBuffer;
		RHI::DescriptorIndex IndexBufferDescIndex;

		RHI::BufferHandle VertexBuffer;
		RHI::DescriptorIndex VertexBufferDescIndex;

		enum class VertexAttribute : uint8_t
		{
			Position = 0,
			TexCoord,
			Normal,
			Tangent,
			Colour,
			Count,
		};
		std::array<RHI::BufferRange, (int)VertexAttribute::Count> BufferRanges;

		[[nodiscard]] bool HasVertexAttribuite(VertexAttribute attr) const { return this->BufferRanges[(int)attr].SizeInBytes != 0; }
		RHI::BufferRange& GetVertexAttribute(VertexAttribute attr) { return this->BufferRanges[(int)attr]; }
		[[nodiscard]] const RHI::BufferRange& GetVertexAttribute(VertexAttribute attr) const { return this->BufferRanges[(int)attr]; }

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

