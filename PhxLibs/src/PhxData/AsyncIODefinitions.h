
#include <string>
#include <vector>
#include <functional>
#include <cstdint>
#include <variant> // For VFSResult or similar error handling

namespace phx::data
{
    // Forward declarations
    struct AsyncReadResult;

    // --- Enums ---
    enum class AsyncDataSourceType 
    {
        Unknown_Or_Error,
        Os_File,
        Pak_Entry
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

    struct AsyncResourceDescriptor 
    {
        AsyncDataSourceType type = AsyncDataSourceType::Unknown_Or_Error;
        std::string os_path_or_pak_path;
        std::string virtual_path;

        uint64_t offset_in_pak = 0;
        uint64_t length_of_resource = 0;

        CompressionInfo compression_info;

        bool IsValid() const 
        {
            return type != AsyncDataSourceType::Unknown_Or_Error && !os_path_or_pak_path.empty();
        }
    };

    struct AsyncReadRequest 
    {
        AsyncResourceDescriptor resource_descriptor;
        uint64_t read_offset_within_resource = 0;
        uint64_t bytes_to_read = 0;

        void* cpu_destination_buffer = nullptr;
        // uint64_t cpu_destination_buffer_size = 0; // Size of the pre-allocated buffer

        std::function<void(const AsyncReadResult& result)> callback;
        void* user_context = nullptr;            // Custom data to be passed to the callback
    };

    struct AsyncReadResult 
    {
        void* user_context = nullptr;
        bool success = false;
        std::vector<uint8_t> data_buffer;
        uint64_t bytes_actually_read = 0;
        std::string error_message;
    };

    // --- VFS Result Type (Example using std::variant for error handling) ---
    template<typename ValueType, typename ErrorType = std::string>
    using Result = std::variant<ValueType, ErrorType>;

    template<typename ValueType, typename ErrorType = std::string>
    bool HasValue(const Result<ValueType, ErrorType>& result) {
        return std::holds_alternative<ValueType>(result);
    }

    template<typename ValueType, typename ErrorType = std::string>
    const ValueType* GetValue(const Result<ValueType, ErrorType>& result) {
        return std::get_if<ValueType>(&result);
    }

    template<typename ValueType, typename ErrorType = std::string>
    const ErrorType* GetError(const Result<ValueType, ErrorType>& result) {
        return std::get_if<ErrorType>(&result);
    }
}