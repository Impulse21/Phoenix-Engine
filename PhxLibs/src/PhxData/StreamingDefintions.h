#pragma once

#include <string>
#include <functional>
#include <cstdint>
#include <variant> 
#include <bitset>

#include <PhxCore/Base.h>
#include <PhxRhi/RHICommon.h>

namespace phx::data
{
    // Forward declarations
    struct StreamingResult;

    // --- Enums ---
    enum class AsyncDataSourceType
    {
        Unknown_Or_Error,
        OS_File,
        Pak_Entry,
        Embedded,
    };

    enum class CompressionMethod
    {
        None,
        GDeflate
    };

    struct CompressionInfo
    {
        CompressionMethod method = CompressionMethod::None;
        uint64_t decompressed_size = 0; // If method != NONE, this is the target size after decompression
    };

    inline uint64_t RequestIdGenerator()
    {
        static constinit std::atomic_uint64_t s_IdCounter = 1;
        return s_IdCounter.fetch_add(1, std::memory_order_relaxed);
    }

    struct AsyncResourceDescriptor
    {
        AsyncDataSourceType type = AsyncDataSourceType::Unknown_Or_Error;

        std::string os_path_or_pak_path;
        std::string virtual_path;

        uint64_t offset_in_pak = 0;
        uint64_t length_of_resource = 0;
        const char* memory_buffer_ptr = nullptr; // For embedded resources

        CompressionInfo compression_info;

        bool IsValid() const
        {
            return type != AsyncDataSourceType::Unknown_Or_Error && !os_path_or_pak_path.empty();
        }
    };

    struct GpuResourceDestinationInfo
    {
        std::variant<RHI::TextureHandle, RHI::GpuBufferHandle> handle;

        bool IsTextureDestination() const { return std::holds_alternative<RHI::TextureHandle>(handle); }
        bool IsBufferDestination() const { return std::holds_alternative<RHI::GpuBufferHandle>(handle); }
    };

    using ReadableCpuMemoryBuffer = std::shared_ptr<const char[]>;
    using WriteableCpuMemoryBuffer = std::shared_ptr<char[]>;
    struct StreamingSource
    {
        std::variant<AsyncResourceDescriptor, ReadableCpuMemoryBuffer> data;
        uint64_t offset = 0;
        uint64_t size = 0;

        bool IsFileSource() const { return std::holds_alternative<AsyncResourceDescriptor>(data); }
        bool IsCpuMemorySource() const { return std::holds_alternative<std::shared_ptr<const char[]>>(data); }
    };

    struct StreamingDestination
    {
        std::variant<WriteableCpuMemoryBuffer, GpuResourceDestinationInfo> target;
        uint64_t offset = 0;
        uint64_t size = 0;

        bool IsCpuMemoryDestination() const { return std::holds_alternative<WriteableCpuMemoryBuffer>(target); }
        bool IsGpuResourceDestination() const { return std::holds_alternative<GpuResourceDestinationInfo>(target); }
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