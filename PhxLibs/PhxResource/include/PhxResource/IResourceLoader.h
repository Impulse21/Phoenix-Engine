#pragma once

#include <PhxResource/IO/IIoQueue.h>
#include <PhxCore/JobSystem.h>

#include <PhxResource/Resource.h>
#include <PhxResource/ResourceTypes.h>
#include <PhxResource/ResourceTypeTraits.h>

namespace phx
{
	class IVirtualFileSystem;
	struct AsyncResourceDescriptor;
	struct StreamingRequest;

    enum class LoaderStepResult
    {
        Continue,
        Yield,
        WaitOnGpuTransition,
        Done,
        Error
    };

    struct LoadContext
    {
        RefCountPtr<Resource> handle;
		AsyncResourceDescriptor resource_descriptor;
        uint8_t state_index = 0;

        IOTicket io_ticket;
        phx::JobSystem::Barrier job_sync;
        MemoryBuffer file_buffer;

        std::vector<RefCountPtr<Resource>> dependencies;

        static constexpr size_t kScratchSize = 128;
        alignas(8) uint8_t scratch_data[kScratchSize];

        // Helper to cast scratch memory
        template<typename T>
        T* GetScratch()
        {
            static_assert(sizeof(T) <= kScratchSize, "Scratch data too large for inline buffer");
            return reinterpret_cast<T*>(scratch_data);
        }

        template<typename T>
        T GetInternalState()
        {
			return static_cast<T>(state_index);
        }
    };

	class IResourceLoader
	{
	public:
		virtual ~IResourceLoader() = default;
		virtual bool IsStale(AsyncResourceDescriptor const& resource_descriptor, IVirtualFileSystem* vfs) const = 0;
		// virtual void PrepareRequest(StreamingRequest& request, GenericHandle handle, phx::IIoQueue* queue, AsyncResourceDescriptor const& resource_descriptor) const = 0;

        virtual LoaderStepResult Step(LoadContext& ctx) const = 0;
        virtual void OnCancel(LoadContext& /*ctx*/) const {}
	};
}