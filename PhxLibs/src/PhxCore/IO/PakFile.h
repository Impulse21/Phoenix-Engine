#pragma once

#include "PhxCore/Base.h"
#include "PhxCore/Span.h"
#include "PhxCore/StringHash.h"

#include "PhxCore/IO/ChunkFile.h"
#include "PhxCore/IO/MemoryRegion.h"

#include <wrl.h>
#include <wrl/wrappers/corewrappers.h>
#include <dstorage.h>

#if false
namespace phx
{
    namespace PakFileFormat
    {
        constexpr uint32_t Version = 1;
        constexpr uint32_t MagicNumber = MakeMagicNum('P', 'X', 'P', 'K');

        /*
                +-----------------------+  <--- Start of File
                |    File Header        |  (Fixed Size)
                |-----------------------|
                |   Asset Entires (N)   | (List of AssetEntries)
                |-----------------------|
                |   String Table (N)    |  (Has, name mappings)
                |-----------------------|
                |   Asset Entry (1)     |  ( Asset ChunkFile )
                |-----------------------|
                |   Asset Entry (1-N)   |
                |-----------------------|
                |   Asset Entry (N)     |
                |-----------------------|
                |   Dependencies Heap   |
                |-----------------------|
                |   String heap         |  (Null terminated string data)
                +-----------------------+
        */
        // Make it 64 bytes so I can expand without changing the Range of the header.
        struct Header
        {
            uint32_t Magic;                     // 'PXPK'
            uint32_t Version;
            uint64_t BuildNumber;               // Build Number is a timestamp
            uint32_t NumEntries;                // Number of assets in the PAK
            uint32_t NumStrings;
            uint32_t EntriesOffset;
            uint32_t DependenciesHeapSize;       // Depenencies Heap located before the string heaps
            uint32_t StringHeapSize;             // String heap located at the end of the file

            uint8_t _FreeSpace[23];
        };
        CompileTimeAssertSize(Header, 64);

		struct AssetEntry
		{
            uint32_t Hash; // Hash of filename for lookup
            uint64_t Offset;
            uint32_t Size;
            uint32_t NumDependiences;
            uint32_t DependenciesOffset;
		};

        struct StringEntry
        {
            uint32_t Hash; // Hash of filename for lookup
            uint32_t Offset; // Regions offset
        };

    }


	extern Microsoft::WRL::ComPtr<IDStorageQueue1> g_dsSystemMemoryQueue;
	extern Microsoft::WRL::ComPtr<IDStorageFactory> g_dsFactory;

	void InitDStorage();

	// Ties together a Win32 event with a Windows Threadpool Wait.
	class EventWait
	{
		Microsoft::WRL::Wrappers::Event m_event;
		TP_WAIT* m_wait;

	public:
		EventWait(void* target, PTP_WAIT_CALLBACK callback)
		{
			m_wait = CreateThreadpoolWait(callback, target, nullptr);
			if (!m_wait)
				std::abort();

			constexpr BOOL manualReset = TRUE;
			constexpr BOOL initialState = FALSE;
			m_event.Attach(CreateEventW(nullptr, manualReset, initialState, nullptr));

			if (!m_event.IsValid())
				std::abort();
		}

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

		~EventWait()
		{
			Close();
		}

		void SetThreadpoolWait()
		{
			ResetEvent(m_event.Get());
			::SetThreadpoolWait(m_wait, m_event.Get(), nullptr);
		}

		bool IsSet() const
		{
			return WaitForSingleObject(m_event.Get(), 0) == WAIT_OBJECT_0;
		}

		void Close()
		{
			if (m_wait)
			{
				WaitForThreadpoolWaitCallbacks(m_wait, TRUE);
				CloseThreadpoolWait(m_wait);
				m_wait = nullptr;
			}
		}

		operator HANDLE()
		{
			return m_event.Get();
		}
	};

	enum class StatusArrayEntry : uint32_t
	{
		Metadata,
		CpuData,
		GpuData,
		NumEntries
	};

	namespace PakStatus
	{
		enum
		{
			Loaded = 0,
			LoadingAssetHeaders = 0x0F,
			LoadingHeader = 0xF0,
			Unloaded = 0xFF
		};
	}

    class PakFile
    {
    public:
		explicit PakFile(std::filesystem::path const& path);
		~PakFile();

		void StartMetadataLoad();

		uint8_t GetStatus() { return m_status.load(); }
		phx::Span<PakFileFormat::AssetEntry> GetEntries() const { return Span(m_assetEntriesData.Get(), m_header.EntriesOffset); }
		
		const char* FindFilenameByHash(phx::StringHash targetHash);

	private:
		void OnHeaderLoaded();
		void OnAssetIndexLoaded();

	private:
		enum class InternalState
		{
			FileOpen,
			LoadingHeader,
			LoadingAssetIndex,
			AssetIndexReady,
			Error
		};

		template<typename T>
		MemoryRegion<T> EnqueueReadMemoryRegion(uint64_t offset, uint32_t size)
		{
			MemoryRegion<T> dest(std::make_unique<char[]>(size));

			//TODO: Add compression support
			DSTORAGE_REQUEST r{};
			r.Options.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
			r.Options.DestinationType = DSTORAGE_REQUEST_DESTINATION_MEMORY;
			r.Source.File.Source = m_dsFile.Get();
			r.Source.File.Offset = offset;
			r.Source.File.Size = size;
			r.Destination.Memory.Buffer = dest.Data();
			r.Destination.Memory.Size = size;
			r.UncompressedSize = r.Destination.Memory.Size;
			r.CancellationTag = reinterpret_cast<uint64_t>(this);

			g_dsSystemMemoryQueue->EnqueueRequest(&r);

			return dest;
		}

		//
		// Enqueues a read of a single, fixed-size, uncompressed piece of data.
		//
		template<typename T>
		void EnqueueRead(uint64_t offset, T* dest)
		{
			DSTORAGE_REQUEST r{};
			r.Options.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
			r.Options.DestinationType = DSTORAGE_REQUEST_DESTINATION_MEMORY;
			r.Options.CompressionFormat = DSTORAGE_COMPRESSION_FORMAT_NONE;
			r.Source.File.Source = m_dsFile.Get();
			r.Source.File.Offset = offset;
			r.Source.File.Size = static_cast<uint32_t>(sizeof(T));
			r.Destination.Memory.Buffer = dest;
			r.Destination.Memory.Size = r.Source.File.Size;
			r.UncompressedSize = r.Destination.Memory.Size;
			r.CancellationTag = reinterpret_cast<uint64_t>(this);

			g_dsSystemMemoryQueue->EnqueueRequest(&r);
		}

		template<typename T>
		void EnqueueReadArray(uint64_t offset, T* dest, size_t numEntries)
		{
			DSTORAGE_REQUEST r{};
			r.Options.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
			r.Options.DestinationType = DSTORAGE_REQUEST_DESTINATION_MEMORY;
			r.Options.CompressionFormat = DSTORAGE_COMPRESSION_FORMAT_NONE;
			r.Source.File.Source = m_dsFile.Get();
			r.Source.File.Offset = offset;
			r.Source.File.Size = static_cast<uint32_t>(sizeof(T) * numEntries);
			r.Destination.Memory.Buffer = dest;
			r.Destination.Memory.Size = r.Source.File.Size;
			r.UncompressedSize = r.Destination.Memory.Size;
			r.CancellationTag = reinterpret_cast<uint64_t>(this);

			g_dsSystemMemoryQueue->EnqueueRequest(&r);
		}

		template<typename... States>
		void ValidateState(States... states) const
		{
			if (!StateIsOneOf(states...))
				throw std::runtime_error("Called in incorrect state");
		}


		template<typename... States>
		bool StateIsOneOf(States... states) const
		{
			for (InternalState state : {states...})
			{
				if (m_state == state)
					return true;
			}

			return false;
		}

	private:
		PakFileFormat::Header m_header;
		MemoryRegion<PakFileFormat::AssetEntry> m_assetEntriesData;
		MemoryRegion<PakFileFormat::StringEntry> m_assetStringEntriesData;
		MemoryRegion<char> m_assetStringHeap;

	private:
		EventWait m_headerLoaded;
		EventWait m_assetIndexLoaded;

		std::atomic_uint8_t m_status;
		mutable std::mutex m_mutex;
		InternalState m_state = InternalState::FileOpen;
		Microsoft::WRL::ComPtr<IDStorageFile> m_dsFile;
		BY_HANDLE_FILE_INFORMATION m_fileInfo = {};
		Microsoft::WRL::ComPtr<IDStorageStatusArray> m_statusArray;
    };
}
#endif