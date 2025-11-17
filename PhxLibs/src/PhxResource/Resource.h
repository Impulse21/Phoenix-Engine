#pragma once

#include <PhxRhi/RHICommon.h>
#include <PhxRhi/PhxRhi_ForwardDeclares.h>
#include <PhxCore/RefCountPtr.h>
#include <PhxCore/StringHash.h>

#define PHX_DECLARE_RESOURCE(TYPE)															\
public:																						\
    static constexpr phx::StringHash StaticTypeHash() { return phx::StringHash(#TYPE); }	\
    TYPE() : Resource(StaticTypeHash()) {}

namespace phx
{
    struct GpuTransitionWork
    {
        struct BufferWork
        {
            rhi::BufferHandle buffer;
            rhi::ResourceStates state;

            uint64_t offset;
            uint64_t size;
        };

        struct TextureWork
        {
            rhi::TextureHandle texture;
            rhi::ResourceStates state;
            int mip;
            int slice;
        };

        std::variant<BufferWork, TextureWork> Data;

        static GpuTransitionWork CreateTexture(
            rhi::TextureHandle texture,
            rhi::ResourceStates state,
            int mip = -1,
            int slice = -1)
        {
            GpuTransitionWork::TextureWork t = {
                .texture = texture,
                .state = state,
                .mip = mip,
                .slice = slice
            };

            GpuTransitionWork work = {
                .Data = t
            };
            return work;
        }

        static GpuTransitionWork CreateBuffer(
            rhi::BufferHandle buffer,
            rhi::ResourceStates state,
            uint32_t size = ~0,
            uint32_t offset = 0)
        {
            GpuTransitionWork::BufferWork b = {
                .buffer = buffer,
                .state = state,
                .offset = offset,
                .size = size
            };

            GpuTransitionWork barrier = {
                .Data = b
            };

            return barrier;
        }
    };

	struct Resource : public RefCounted
	{
		const uint64_t resource_type_hash;
		
		enum State : uint8_t
		{
			Loaded = 0,
			Final_GPU_Transition = 0x01,
			On_Gpu = 0x02,
			Loading = 0x0F,
			Error = 0x7F,
			Unloaded = 0xFF
		};
		std::atomic_uint8_t state = State::Unloaded;

		virtual bool CollectPendingGpuTransitions(SpanMutable<GpuTransitionWork> transitions, size_t& fill_index) = 0;
        virtual ~Resource() = default;
		bool IsLoaded()
		{
			return state == State::Loaded;
		}

    protected:
        explicit Resource(uint64_t hash) : resource_type_hash(hash) {}

	};

}