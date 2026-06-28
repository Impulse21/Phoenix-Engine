#pragma once

#include <PhxEngine/Memory/FrameAllocator.h>
#include <PhxEngine/Memory/ScratchAllocator.h>
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


	struct PassResult
	{
		int index;

		PassResult() {}
		explicit PassResult(const int index) : index(index) {}

		constexpr Reference Colour(const int index, const bool entire_texture = false) const { return Reference(ReferenceType::PassResult, index, index, 0, entire_texture); }
		constexpr Reference Depth(const bool entire_texture = false) const { return Reference(ReferenceType::PassResult, index, 0, 1, entire_texture); }
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
	}

	struct PassDesc
	{
		const char* name = "";
		PassCallbackFn callback = nullptr;
		void* user_data = nullptr;

		std::vector<Reference> references;
	};

	class CompiledRenderGraph
	{
	public:
		void Execute();

	};

	class PassBuilder
	{
	public:
		Reference Read(Reference ref) {}
		Reference Write(Reference ref) {}

	private:
		friend class RenderGraphBuilder;
		PassBuilder(PassDesc* desc)
			: m_desc(desc)
		{ }

		PassDesc* m_desc;
	};
    
	class RenderGraphBuilder
	{
	public:
		static RenderGraphBuilder Create() { return RenderGraphBuilder(); }
	public:
		RenderGraphBuilder() = default;
		~RenderGraphBuilder() = default;

		GraphResource DeclareResource(const ResourceDesc) { return {}; }
		GraphResource GetBackBuffer() { return {}; }

		template<typename TSetupFn>
		void AddPass(
			const std::string& pass_name,
            TSetupFn&& setupFn,
			PassCallbackFn pass_callback)
		{
			PassDesc& desc = m_passDesc.emplace_back();
			PassBuilder builder(builder);
			setupFn(builder);
		}

	public:
		[[nodiscard]] CompiledRenderGraph* Compile();
		std::vector<PassDesc> m_passDesc;
	};
}