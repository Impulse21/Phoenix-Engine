#pragma once

#include <PhxEngine/Core/CVar.h>
#include <PhxEngine/Core/FixedCallable.h>

#include <PhxEngine/Memory/FrameAllocator.h>

#include <PhxEngine/Renderer/RenderGraphRegistry.h>

#include <PhxEngine/RHI/RHITypes.h>
#include <PhxEngine/RHI/GpuMemory/TextureAllocator.h>

// -- Forward Declarations ---
namespace phx
{
	namespace renderer
	{
		struct ResourceEntry;
	}
}

// -- API ---
namespace phx::renderer
{
	enum class ReferenceType : unsigned int
	{
		Invalid,
		GraphResource,
		PassResult,
		ExternalSRV
	};

	struct Reference
	{
		enum : uint8_t
		{
			AllSubresources = 0x7f
		};

		union
		{
			struct
			{
				ReferenceType	type				: 2;
				unsigned int	depth				: 1;
				unsigned int	entire_texture		: 1;		 // Used for PassResult types to specify AllSubresources (since SubResource is used for the output index)
				unsigned int	mip_slice			: 1;
				unsigned int	sub_resource		: 7;
				unsigned int	mip_level			: 4;
				unsigned int	index				: 16;
			};
			unsigned int data;
		};

		Reference() {}

		constexpr Reference(const ReferenceType type, const unsigned int index, const unsigned int sub_resource, const unsigned int depth = 0, const unsigned int entire_texture = 0, const unsigned int mip_slice = 0, const unsigned int mip_level = 0) :
			type(type),
			depth(depth),
			entire_texture(entire_texture),
			mip_slice(mip_slice),
			sub_resource(sub_resource),
			mip_level(mip_level),
			index(index)
		{
			assert(sub_resource < 128);
			assert(index < 65536);
			assert(mip_slice == 1 || mip_level == 0);
			assert(mip_level < 16);
		}

		constexpr Reference SubResourceRef(const unsigned int sub_resource) const { return Reference(type, index, sub_resource, depth); }
		constexpr Reference MipSliceRef(const unsigned int mip_level) const { return Reference(type, index, 0, depth, 0, 1, mip_level); }

		static constexpr Reference Null()
		{
			return Reference(ReferenceType::Invalid, 0, 0);
		}

		explicit operator bool() const { return data != 0; }
	};

	struct GraphResource
	{
		int index;

		GraphResource() {}
		explicit GraphResource(const int index) : index(index) {}

		constexpr operator Reference() const { return Reference(ReferenceType::GraphResource, index, 0); }
		constexpr Reference SubResource(const unsigned int sub_resource) const { return Reference(ReferenceType::GraphResource, index, sub_resource); }
		constexpr Reference MipSlice(const unsigned int mip_level) const { return Reference(ReferenceType::GraphResource, index, 0, 0, 0, 1, mip_level); }
		constexpr Reference AllSubResources() const { return SubResource(Reference::AllSubresources); }
	};

	class PassContext
	{
	public:
		rhi::PlacedTexture ResolveTexture(Reference /*ref*/) { return {}; }
		rhi::DescriptorIndex ResolveTextureDescriptor(Reference /*ref*/) { return ~0U; }
		rhi::GpuRange ResolveBuffer() { return {}; }
	};

	using PassExecFn = FixedCallable<128, void(phx::rhi::CommandBuffer, PassContext&)>;


	struct Usage
	{
		rhi::BarrierStage  stage;
		rhi::BarrierAccess access;
	};

	constexpr Usage operator|(Usage a, Usage b) { return { a.stage | b.stage, a.access | b.access }; }

	namespace usage   // presets so call sites stay short, and stage/access can't be mismatched by accident
	{
		inline constexpr Usage ColorTarget  { rhi::BarrierStage::ColorOutput,       rhi::BarrierAccess::ColorWrite };
		inline constexpr Usage DepthTarget  { rhi::BarrierStage::DepthStencilTests, rhi::BarrierAccess::DepthStencilWrite };
		inline constexpr Usage FragmentRead { rhi::BarrierStage::Fragment,          rhi::BarrierAccess::ShaderRead };
		inline constexpr Usage MeshRead     { rhi::BarrierStage::Mesh,              rhi::BarrierAccess::ShaderRead };
		inline constexpr Usage ComputeRead  { rhi::BarrierStage::Compute,           rhi::BarrierAccess::ShaderRead };
		inline constexpr Usage ComputeWrite { rhi::BarrierStage::Compute,           rhi::BarrierAccess::ShaderWrite };
	}

	struct ResourceAccess { Reference ref; Usage usage; };

	struct PassDesc
	{
		PassExecFn 	execute_fn = nullptr;

		FramePtr<ResourceAccess> reads = nullptr;
		FramePtr<ResourceAccess> writes = nullptr;
		
		u32 read_count = 0;
		u32 write_count = 0;

		u32 read_capacity = 0;
		u32 write_capacity = 0;

		const char* name = "";
		bool is_trival = false;
	};

	// -- Fluent pass builder ---
	class PassBuilder
	{
	public:
		~PassBuilder()
		{
			PHX_ASSERT(m_desc.execute_fn.IsValid() && "Pass never finished: missing call to EndPass");
		}

		PassBuilder& Trival()
		{
			m_desc.is_trival = true;
			return *this;
		}
		
		PassBuilder& Read(ResourceAccess access)
		{
			PHX_ASSERT(m_desc.read_count < m_desc.read_capacity);
			m_desc.reads[m_desc.read_count++] = access;

			return *this;
		}

		PassBuilder& Write(ResourceAccess access)
		{
			PHX_ASSERT(m_desc.write_count < m_desc.write_capacity);
			m_desc.writes[m_desc.write_count++] = access;

			return *this;
		}

		void EndPass(PassExecFn exec_fn)
		{
			PHX_ASSERT(exec_fn.IsValid());
			m_desc.execute_fn = std::move(exec_fn);
		}

	private:
		friend class RenderGraphBuilder;
		explicit PassBuilder(PassDesc& desc)
			: m_desc(desc)
		{
		}

		PassDesc& m_desc;
	};


	struct RenderGraphLimits
	{
		u32 max_passes 			= 64;
		u32 max_resources 		= 256;
		u32 max_reads_per_pass 	= 16;
		u32 max_writes_per_pass	= 16;
	};

	class RenderGraphBuilder
	{
	public:
		RenderGraphBuilder(renderer::RenderGraphRegistry& registry, FrameAllocator& frame_allocator, const RenderGraphLimits& limits = {});

		void Execute(ScratchAllocator& scratch_allocator);

	public:

		GraphResource DeclareResource(const ResourceDesc& resource_desc);
		GraphResource ImportTexture(rhi::TextureHandle texture);
		GraphResource ImportBuffer(rhi::GpuRange buffer);
		GraphResource ImportSwaphchain();

		PassBuilder BeginPass(const char* pass_name);

	private:
		RenderGraphRegistry* m_registry;
		FrameAllocator* m_frame_allocator;

		FramePtr<ResourceEntry> m_resources;
		u32 m_resource_count = 0;
		u32 m_resource_capacity = 0;

		FramePtr<PassDesc> m_passes;
		u32 m_pass_count = 0;
		u32 m_pass_capacity = 0;
	};
}