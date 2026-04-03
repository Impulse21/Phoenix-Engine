#pragma once

#include <PhxResource/IO/IIoQueue.h>
#include <PhxCore/TaskScheduler.h>

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
        // -- Depericate and use VFS directly --
		AsyncResourceDescriptor resource_descriptor;
        uint8_t state_index = 0;

        IOTicket io_ticket;
        phx::ThreadPoolHandle thread_pool_handle;
        phx::TaskScheduler::Barrier job_sync;
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

    struct NoOpOnComplete { void operator()() const {} };

    template<typename OnCompleteFn = NoOpOnComplete>
    inline LoaderStepResult PollBarrierTask(const LoadContext& ctx, OnCompleteFn&& on_complete = {})
    {
        if (ctx.job_sync.IsNotCleared())
        {
            return LoaderStepResult::Yield;
        }

        on_complete();
        return LoaderStepResult::Continue;
    }

    // -- Helper function to poll io queue in a loader step ---
    template<typename OnSuccessFn, typename OnErrorFn> 
    inline LoaderStepResult PollQueueTask(
        const LoadContext& ctx, 
        IIoQueue* queue,
        OnSuccessFn&& on_success,
        OnErrorFn&& on_error)
    {
        if (!queue->IsComplete(ctx.io_ticket))
        {
            return LoaderStepResult::Yield;
        }
        auto result = queue->GetResult(ctx.io_ticket);
        if (result.error_code != ErrorCode::Success)
        {
            on_error();
        }

        on_success();
        return LoaderStepResult::Continue;
    }

    inline LoaderStepResult PollDependenciesCompleted(const LoadContext& ctx)
    {
        bool all_deps_loaded = true;
        bool has_error = false;
        for (const RefCountPtr<Resource>& dep_handle : ctx.dependencies)
        {
            if (dep_handle->state == ResourceState::Error)
            {
                has_error = true;
                break;
            }

            if (dep_handle->state != ResourceState::Loaded)
            {
                all_deps_loaded = false;
                break;
            }
        }

        if (has_error)
        {
            return LoaderStepResult::Error;
        }

        if (!all_deps_loaded)
        {
            return LoaderStepResult::Yield;
        }

        return LoaderStepResult::Done;
    }

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