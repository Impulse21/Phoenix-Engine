#pragma once

#include <PhxEngine/IO/IIoQueue.h>

#include <PhxResource/ResourceTypes.h>
#include <PhxResource/ResourceTypeTraits.h>

namespace phx
{
	class IVirtualFileSystem;
	struct AsyncResourceDescriptor;
	struct StreamingRequest;

    enum class LoaderStepResult
    {
        Continue,   // State advanced, run again immediately (don't sleep)
        Yield,      // Waiting on IO/Job/Dependency. Check again next frame.
        Done,       // Loading finished. Ready for GPU Handoff.
        Error       // Failed.
    };

    struct LoadContext
    {
        GenericHandle handle;
        std::string virtual_file_path;
        uint8_t state_index = 0;

        // IOTicket io_ticket;
        //std::vector<uint8_t> file_buffer; // Raw data from disk (or use a dedicated Blob type)

        std::vector<GenericHandle> dependencies;

        static constexpr size_t kScratchSize = 128;
        alignas(8) uint8_t scratch_data[kScratchSize];

        // Helper to cast scratch memory
        template<typename T>
        T* GetScratch() 
        {
            static_assert(sizeof(T) <= kScratchSize, "Scratch data too large for inline buffer");
            return reinterpret_cast<T*>(scratch_data);
        }
    };

	class IResourceLoader
	{
	public:
		virtual ~IResourceLoader() = default;
		virtual bool IsStale(AsyncResourceDescriptor const& resource_descriptor, IVirtualFileSystem* vfs) const = 0;
		// virtual void PrepareRequest(StreamingRequest& request, GenericHandle handle, phx::IIoQueue* queue, AsyncResourceDescriptor const& resource_descriptor) const = 0;

        virtual LoaderStepResult Step(LoadContext& ctx) = 0;
        virtual void OnCancel(LoadContext& ctx) {}
	};
}