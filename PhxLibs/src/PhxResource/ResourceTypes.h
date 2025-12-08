#pragma once

#include <PhxRhi/PhxRhi_Types.h>

#include <PhxCore/Handle.h>
#include <atomic>
#include <bit>
#include <string>

namespace phx
{
    enum ResourceState : uint8_t
    {
        Loaded = 0,
        Final_GPU_Transition = 0x01,
        On_Gpu = 0x02,
        Loading = 0x0F,
        Error = 0x7F,
        Unloaded = 0xFF
    };

    struct ResourceHotData
    {
        ResourceState state = ResourceState::Unloaded;
    };

    struct ResourceColdData
    {
        std::atomic_uint32_t ref_count = 0;
        std::string source_path;
        std::string debug_name;
    };

    namespace internal
    {
        struct ResourceTypeIdCounter
        {
            static inline std::atomic_uint16_t value = 0;
        };
    }

    template<typename T>
    struct ResourceTypeId
    {
        static uint16_t Get()
        {
            static uint16_t id = internal::ResourceTypeIdCounter::value.fetch_add(1);
            return id;
        }
    };

    struct GenericHandle
    {
        uint16_t type_id = 0;
        uint16_t generation = 0;
        uint16_t index = 0;

        bool IsValid() const { return generation != 0; }

        template<typename T>
        static GenericHandle From(Handle<T> h)
        {
            struct Raw { uint16_t idx; uint16_t gen; };
            Raw raw = std::bit_cast<Raw>(h);

            return { ResourceTypeId<T>::Get(), raw.gen, raw.idx };
        }

        template<typename T>
        Handle<T> To() const
        {
            if (type_id != ResourceTypeId<T>::Get())
            {
                return Handle<T>();

            }
            struct Raw { uint16_t idx; uint16_t gen; };
            Raw raw = { index, generation };
            return std::bit_cast<Handle<T>>(raw);
        }
    };


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

}