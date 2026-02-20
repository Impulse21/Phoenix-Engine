#pragma once

#include <string>
#include <functional>
#include <cstdint>
#include <variant> 
#include <bitset>

#include <PhxCore/Base.h>
#include <PhxRhi/PhxRhi.h>
#include <PhxCore/IO/MemoryRegion.h>
#include <PhxCore/IVirtualFileSystem.h>

namespace phx
{
    // Forward declarations
    struct StreamingResult;
    struct GpuTextureDestination
    {
        rhi::TextureHandle handle;
        uint32_t mip_level = rhi::c_remaning_mip_levels;
        uint32_t array_layer = rhi::c_remaning_array_layers;
    };

    struct GpuBufferDestination
    {
        rhi::BufferHandle handle;
        uint64_t offset = 0;
    };

    struct CpuDestination
    {
        void* address = nullptr;
    };

    using ReadableCpuMemoryBuffer = const std::byte*;
    struct StreamingSource
    {
        std::variant<AsyncResourceDescriptor, ReadableCpuMemoryBuffer> data;
        uint64_t offset = 0;
        uint64_t size = 0;

        bool IsFileSource() const { return std::holds_alternative<AsyncResourceDescriptor>(data); }
        bool IsCpuMemorySource() const { return std::holds_alternative<ReadableCpuMemoryBuffer>(data); }
    };

    struct StreamingDestination
    {
        std::variant<CpuDestination, GpuBufferDestination, GpuTextureDestination> target;
        uint64_t size = 0;

        bool IsCpu() const { return std::holds_alternative<CpuDestination>(target); }
        bool IsGpuBuffer() const { return std::holds_alternative<GpuBufferDestination>(target); }
        bool IsGpuTexture() const { return std::holds_alternative<GpuTextureDestination>(target); }
    };

    struct StreamingOperation
    {
        StreamingSource source;
        StreamingDestination destination;
    };

    struct StreamingRequest
    {
        const char* debug_name = "";
        uint64_t request_id = 0;

        std::vector<StreamingOperation> operations;

        std::function<void(StreamingResult const& result)> on_complete;

    };

    enum class ErrorCode
    {
        Success = 0,
        InvalidSource,
        InvalidDestination,
        Unknown,
    };

    struct StreamingResult 
    {
        uint64_t request_id = 0;
        std::bitset<8> status_array;
        ErrorCode error_code = ErrorCode::Unknown;
    };
}