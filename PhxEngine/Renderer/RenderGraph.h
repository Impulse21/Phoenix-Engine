#pragma once

#include <PhxEngine/Core/CVar.h>

#include <PhxEngine/Memory/FrameAllocator.h>
#include <PhxEngine/Memory/ScratchAllocator.h>
#include <PhxEngine/Memory/MemoryHelpers.h>

#include <PhxEngine/RHI/RHITypes.h>

namespace phx::renderer
{
    enum class ResourceKind : u8
    {
        Texture,
        Buffer
    };

    struct TextureDesc
    {
        u32 width = 0;
        u32 height = 0;
        rhi::Format format = rhi::Format::UNKNOWN;
        const char* debug_name = "";
    };

    struct BufferDesc
    {
        usize sizeBytes = 0;
        const char* debug_name = "";
    };

    struct ResourceDesc
    {
        ResourceKind kind = ResourceKind::Texture;
        union
        {
            TextureDesc texture;
            BufferDesc buffer;
        };

        static constexpr ResourceDesc Texture(TextureDesc t)
        {
            ResourceDesc d{};
            d.kind = ResourceKind::Texture;
            d.texture = t;
            return d;
        }
        
        static constexpr ResourceDesc Buffer(BufferDesc b)
        {
            ResourceDesc d{};
            d.kind = ResourceKind::Buffer;
            d.buffer = b;
            return d;
        }
    };


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

	using PassCallbackFn = void(*)(rhi::CommandBufferHandle);

	struct ResourceEntry
	{
		ResourceDesc desc;
		union 
		{
			rhi::TextureHandle external_texture;
			rhi::GpuBufferHandle external_gpu_buffer;
		};

		bool is_back_buffer = false;
		bool is_imported = false;
		
		rhi::ResourceStates current_layout = rhi::ResourceStates::Common;
	};

	struct PassDesc
	{
		const char* name = "";
		PassCallbackFn callback = nullptr;

		FramePtr<Reference> reads = nullptr;
		FramePtr<Reference> writes = nullptr;

		u32 read_count = 0;
		u32 write_count = 0;
	};

	class CompiledRenderGraph
	{
	public:
		void Execute();
	};


	PHX_XCVAR_INT(rg_max_reads_per_pass);
	PHX_XCVAR_INT(rg_max_writes_per_pass);
	class PassBuilder
	{
	public:
		Reference Read(Reference ref)
		{
			PHX_ASSERT(
				m_desc->read_count < CVar_rg_max_reads_per_pass.Get(),
				"Exceeded maximum number of reads in a single pass");

			m_desc->reads[m_desc->read_count++] = ref;
			return ref;
		}

		Reference Write(Reference ref)
		{
			PHX_ASSERT(
				m_desc->write_count < CVar_rg_max_writes_per_pass.Get(),
				"Exceeded maximum number of writes in a single pass");

			m_desc->writes[m_desc->write_count++] = ref;
			return ref;
		}

	private:
		friend class RenderGraphBuilder;
		PassBuilder(FrameAllocator& frame_alloc, PassDesc* desc);

		PassDesc* m_desc;
	};
    
	class RenderGraphBuilder
	{
	public:
		static RenderGraphBuilder Create(FrameAllocator* frame_alloc) 
		{ 
			return RenderGraphBuilder(frame_alloc); 
		}

	public:
		RenderGraphBuilder(FrameAllocator* frame_alloc);
		~RenderGraphBuilder() = default;

		GraphResource DeclareResource(const ResourceDesc);
		GraphResource GetBackBuffer();

		template<typename TSetupFn>
		void AddPass(
			const std::string& pass_name,
            TSetupFn&& setupFn,
			PassCallbackFn pass_callback)
		{
			PHX_ASSERT(
				m_pass_count < m_pass_capacity - 1,
				"Exceeded maximum number of passes in render graph");

    		PassDesc* pass_desc = &m_passes[m_pass_count++]
			PassBuilder builder(*m_frame_alloc, pass_desc);

			setupFn(builder);
		}

	private:
		PassDesc* AllocatePassDesc();

	private:
		[[nodiscard]] FramePtr<renderer::CompiledRenderGraph> Compile();
		FrameAllocator* m_frame_alloc;

		FramePtr<PassDesc> m_passes;
		FramePtr<ResourceEntry> m_resources;

		const u32 m_pass_capacity = 0;
		u32 m_pass_count = 0;
		u32 m_resource_count = 0;
	}; 
}