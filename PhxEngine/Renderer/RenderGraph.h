#pragma once

#if false

#include <PhxEngine/Core/CVar.h>

#include <PhxEngine/Memory/FrameAllocator.h>
#include <PhxEngine/Memory/ScratchAllocator.h>
#include <PhxEngine/Memory/MemoryHelpers.h>

#include <PhxEngine/RHI/RHITypes.h>

PHX_XCVAR_INT(rg_max_reads_per_pass);
PHX_XCVAR_INT(rg_max_writes_per_pass);

namespace phx::renderer
{
    enum class ResourceKind : u8
    {
        Texture,
        Buffer
    };

    struct TextureDesc
    {
        u32 width;
        u32 height;
        rhi::Format format;
        const char* debug_name;
    };

    struct BufferDesc
    {
        usize sizeBytes;
        const char* debug_name;
    };

    struct ResourceDesc
    {
        ResourceKind kind = ResourceKind::Texture;
        union
        {
            TextureDesc texture;
            BufferDesc buffer;
        };

		ResourceDesc() = default;

        static ResourceDesc Texture(TextureDesc t)
        {
            ResourceDesc d = {};
            d.kind = ResourceKind::Texture;
            d.texture = t;
            return d;
        }
        
        static ResourceDesc Buffer(BufferDesc b)
        {
            ResourceDesc d = {};
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

		ResourceEntry()
			: desc{}
			, external_texture{}
		{}
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
		friend class RenderGraphBuilder;
	public:
		void Execute() {}

	private:
		FramePtr<PassDesc> m_passes;
		FramePtr<ResourceEntry> m_resources;
		u32 m_pass_count = 0;
		u32 m_resource_count = 0;
	};
	
	class PassBuilder
	{
	public:
		Reference Read(Reference ref)
		{
			PHX_ASSERT(
				m_desc->read_count < static_cast<u32>(CVar_rg_max_reads_per_pass.Get()));

			m_desc->reads[m_desc->read_count++] = ref;
			return ref;
		}

		Reference Write(Reference ref)
		{
			PHX_ASSERT(
				m_desc->write_count < static_cast<u32>(CVar_rg_max_writes_per_pass.Get()));

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
		static constexpr int k_backbuffer_index = INT32_MAX;
	public:
		static FramePtr<renderer::RenderGraphBuilder> Create(FrameAllocator* frame_alloc) 
		{ 
			return phx::Memory::FrameNew<RenderGraphBuilder>(Memory::g_Frame, frame_alloc);
		}

	public:
		RenderGraphBuilder(FrameAllocator* frame_alloc);
		~RenderGraphBuilder() = default;

		GraphResource DeclareResource(const ResourceDesc& resource_desc);
		GraphResource GetBackBuffer()
		{
			return GraphResource(k_backbuffer_index);
		}

		template<typename TSetupFn>
		void AddPass(
			const char* pass_name,
            TSetupFn&& setupFn,
			PassCallbackFn pass_callback)
		{
			PHX_ASSERT(m_pass_count < m_pass_capacity - 1);

    		PassDesc* pass_desc = &m_passes[m_pass_count++];
			pass_desc->name = pass_name;
			pass_desc->callback = pass_callback;

			PassBuilder builder(*m_frame_alloc, pass_desc);

			setupFn(builder);
		}

		[[nodiscard]] FramePtr<renderer::CompiledRenderGraph> Compile();

	private:
		PassDesc* AllocatePassDesc();

		const ResourceEntry& ResolveReference(Reference ref) const { return m_resources[ref.index]; };

	private:
		FrameAllocator* m_frame_alloc;

		FramePtr<PassDesc> m_passes;
		FramePtr<ResourceEntry> m_resources;

		const u32 m_pass_capacity = 0;
		u32 m_pass_count = 0;
		u32 m_resource_count = 0;
	}; 
}

#endif