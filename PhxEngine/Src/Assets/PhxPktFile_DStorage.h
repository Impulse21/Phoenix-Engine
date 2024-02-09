#pragma once

#include <PhxEngine/Core/RefCountPtr.h>
#include <PhxEngine/Assets/PhxArchFile.h>
#include <PhxEngine/Assets/PhxArchFormat.h>
#include <PhxEngine/Core/MemoryRegion.h>

#include <mutex>
#include <dstorage.h>

#include <wrl/wrappers/corewrappers.h>

#include <cstdlib>
#include <functional>

using namespace PhxEngine;
using namespace PhxEngine::Core;
using namespace PhxEngine::Assets;

namespace PhxEngine::Assets
{

    class EventWait
    {
    public:
        template<typename T, void (T::* FN)()>
        static EventWait Create(T* target)
        {
            auto callback = [](TP_CALLBACK_INSTANCE*, void* context, TP_WAIT*, TP_WAIT_RESULT)
                {
                    T* target = reinterpret_cast<T*>(context);
                    (target->*FN)();
                };

            return EventWait(target, callback);
        }

    public:
        EventWait(void* target, PTP_WAIT_CALLBACK callback)
        {
            this->m_wait = CreateThreadpoolWait(callback, target, nullptr);
            if (!this->m_wait)
                std::abort();

            constexpr BOOL manualReset = TRUE;
            constexpr BOOL initialState = FALSE;
            this->m_event.Attach(CreateEventW(nullptr, manualReset, initialState, nullptr));

            if (!this->m_event.IsValid())
                std::abort();
        }

        ~EventWait()
        {
            Close();
        }

        void SetThreadpoolWait()
        {
            ResetEvent(this->m_event.Get());
            ::SetThreadpoolWait(this->m_wait, this->m_event.Get(), nullptr);
        }

        bool IsSet() const
        {
            return WaitForSingleObject(this->m_event.Get(), 0) == WAIT_OBJECT_0;
        }

        void Close()
        {
            if (this->m_wait)
            {
                WaitForThreadpoolWaitCallbacks(this->m_wait, TRUE);
                CloseThreadpoolWait(this->m_wait);
                this->m_wait = nullptr;
            }
        }

        operator HANDLE()
        {
            return this->m_event.Get();
        }

    private:
        Microsoft::WRL::Wrappers::Event m_event;
        TP_WAIT* m_wait;
    };

	class PhxPktFile_DStorage : public PhxPktFile
	{
	public:
        PhxPktFile_DStorage(std::string const& filename);

		void StartMetadataLoad() override;

	private:
		mutable std::mutex m_mutex;

		Core::RefCountPtr<IDStorageFile> m_file;
		Core::RefCountPtr<IDStorageStatusArray> m_statusArray;

		// Metadata
		PhxArch::Header header = {};
		MemoryRegion<PhxArch::AssetEntriesHeader> m_entries;

		// Constructed Heaps? maybe this goes else where...

		// Contents

		enum class StatusArrayEntry : uint32_t
		{
			Metadata,
			CpuData,
			GpuData,
			NumEntries
		};

		enum class InternalState
		{
			FileOpen,
			LoadingHeader,
			LoadingCpuMetadata,
			MetadataReady,
			LoadingContent,
			CpuDataLoaded,
			GpuDataLoaded,
			ContentLoaded,
			Error
		};
		InternalState m_state = InternalState::FileOpen;

		// EventWait m_headerLoaded;
		// EventWait m_cpuMetadataLoaded;
	};
}

