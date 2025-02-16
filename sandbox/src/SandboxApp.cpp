#include <Phoenix.h>
#include <mutex>
#include <atomic>

#include "PhxCore/EntryPoint.h"

#include "PhxCore/VFS.h"
#include "PhxRenderer/ImGuiRenderer.h"


#include "PhxCore/IO/PakFile.h"
#include "PhxCore/IO/MemoryRegion.h"

#include <wrl/wrappers/corewrappers.h>


#include <dstorage.h>

using namespace phx;


namespace
{
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

	Microsoft::WRL::ComPtr<IDStorageQueue1> g_dsSystemMemoryQueue;
	Microsoft::WRL::ComPtr<IDStorageFactory> g_dsFactory;

	void Test_InitDStorage()
	{
		// Load a file and init D3D12
		HRESULT hr = DStorageGetFactory(IID_PPV_ARGS(&g_dsFactory));
		PHX_ASSERT(SUCCEEDED(hr));
		g_dsFactory->SetDebugFlags(DSTORAGE_DEBUG_BREAK_ON_ERROR | DSTORAGE_DEBUG_SHOW_ERRORS);
		g_dsFactory->SetStagingBufferSize(256 * 1024 * 1024);


		// Create a DirectStorage queue which will be used to load data into a
		// buffer on the GPU.

		// Create the system memory queue, used for reading data into system memory.
		{
			DSTORAGE_QUEUE_DESC queueDesc{};
			queueDesc.Capacity = DSTORAGE_MAX_QUEUE_CAPACITY;
			queueDesc.Priority = DSTORAGE_PRIORITY_NORMAL;
			queueDesc.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
			queueDesc.Name = "SysMemoryQueue";

			hr = (g_dsFactory->CreateQueue(&queueDesc, IID_PPV_ARGS(&g_dsSystemMemoryQueue)));
			PHX_ASSERT(SUCCEEDED(hr));
		}
	}

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

	class PakFile_Test
	{
	public:
		PakFile_Test(std::filesystem::path const& path)
			: m_headerLoaded(EventWait::Create<PakFile_Test, &PakFile_Test::OnHeaderLoaded>(this))
			, m_assetIndexLoaded((EventWait::Create<PakFile_Test, &PakFile_Test::OnAssetIndexLoaded>(this)))
		{
			HRESULT hr = g_dsFactory->OpenFile(path.wstring().c_str(), IID_PPV_ARGS(&m_dsFile));
			if (FAILED(hr))
			{
				std::string outMsg;
				StringConvert(path.wstring().c_str(), outMsg);
				PHX_ERROR("The file {0}, could no open.", outMsg);
				return;
			}


			BY_HANDLE_FILE_INFORMATION info{};
			hr = (m_dsFile->GetFileInformation(&info));
			PHX_ASSERT(SUCCEEDED(hr));
			uint32_t fileSize = info.nFileSizeLow;
			PHX_INFO("FileSize {0}.", fileSize);

			hr = (g_dsFactory->CreateStatusArray(
				static_cast<uint32_t>(StatusArrayEntry::NumEntries),
				nullptr,
				IID_PPV_ARGS(&m_statusArray)));

			PHX_ASSERT(SUCCEEDED(hr));

			m_status.store(0xFF);
		}

		~PakFile_Test()
		{
			// All requests created for this instance are tagged with 'this', so we can
			// cancel any outstanding requests.
			g_dsSystemMemoryQueue->CancelRequestsWithTag(0xFFFFFFFFFFFFll, reinterpret_cast<uint64_t>(this));
		}

		void StartMetadataLoad()
		{
			std::scoped_lock _(m_mutex);

			ValidateState(InternalState::FileOpen);

			EnqueueRead(0, &m_header);

			m_headerLoaded.SetThreadpoolWait();
			g_dsSystemMemoryQueue->EnqueueStatus(m_statusArray.Get(), static_cast<uint32_t>(StatusArrayEntry::Metadata));
			g_dsSystemMemoryQueue->EnqueueSetEvent(m_headerLoaded);
			g_dsSystemMemoryQueue->Submit();

			m_state = InternalState::LoadingHeader;
			m_status = PakStatus::LoadingHeader;
		}

		uint8_t GetStatus() { return m_status.load(); }
		phx::Span<PakFileFormat::AssetEntry> GetEntries() const { return Span(m_assetEntriesData.Get(), m_header.AssetCount); }

	private:
		void OnHeaderLoaded()
		{
			std::scoped_lock _(m_mutex);

			ValidateState(InternalState::LoadingHeader);

			uint32_t status = m_statusArray->GetHResult(static_cast<uint32_t>(StatusArrayEntry::Metadata));

			if (m_header.Magic != PakFileFormat::MagicNumber ||
				m_header.Version != PakFileFormat::Version ||
				FAILED(status))
			{
				m_state = InternalState::Error;
				return;
			}

			EnqueueReadMemoryRegion(m_header.AssetEntires);
			m_assetIndexLoaded.SetThreadpoolWait();
			g_dsSystemMemoryQueue->EnqueueSetEvent(m_assetIndexLoaded);
			g_dsSystemMemoryQueue->Submit();

			m_state = InternalState::LoadingAssetIndex;
			m_status = PakStatus::LoadingAssetHeaders;
		}

		void OnAssetIndexLoaded()
		{
			std::scoped_lock _(m_mutex);

			ValidateState(InternalState::LoadingAssetIndex);
			m_status = PakStatus::Loaded;

		}

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
		MemoryRegion<T> EnqueueReadMemoryRegion(PakFileFormat::PakRegion<T> const& region)
		{
			MemoryRegion<T> dest(std::make_unique<char[]>(region.Size));

			DSTORAGE_REQUEST r{};
			r.Options.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
			r.Options.DestinationType = DSTORAGE_REQUEST_DESTINATION_MEMORY;
			r.Options.CompressionFormat = ToCompressionFormat(region.Compression);
			r.Source.File.Source = m_dsFile.Get();
			r.Source.File.Offset = region.Data.Offset;
			r.Source.File.Size = region.CompressedSize;
			r.Destination.Memory.Buffer = dest.Data();
			r.Destination.Memory.Size = region.UncompressedSize;
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

	private:
		EventWait m_headerLoaded;
		EventWait m_assetIndexLoaded;

		std::atomic_uint8_t m_status;
		mutable std::mutex m_mutex;
		InternalState m_state = InternalState::FileOpen;
		Microsoft::WRL::ComPtr<IDStorageFile> m_dsFile;
		Microsoft::WRL::ComPtr<IDStorageStatusArray> m_statusArray;
	};
}
class Sandbox final : public phx::IApplication
{
public:
	static Sandbox* Instance() { return ms_instance; }

public:
	Sandbox(const phx::ApplicationDescriptor& desc)
		: m_desc(desc)
	{
		ms_instance = this;
	}

	~Sandbox() { ms_instance = nullptr; }

	void Startup() override
	{
		PHX_INFO("Sandbox app is starting up");
		m_fs = phx::FileSystemFactory::CreateRootFileSystem();

		std::filesystem::path applicationShaderPath = phx::VFS::GetDirectoryWithExecutable() / "shaders/application";
		std::filesystem::path frameworkShaderPath = phx::VFS::GetDirectoryWithExecutable() / "shaders/engine";

		m_fs->Mount("/native", phx::FileSystemFactory::CreateNativeFileSystem());
		m_fs->Mount("/shaders", applicationShaderPath);
		m_fs->Mount("/shaders_engine", frameworkShaderPath);

		m_imguiRenderer.Initialize(m_fs.get(), m_windowHandle);
		m_imguiRenderer.EnableDarkThemeColours();

		Test_InitDStorage();
		const wchar_t* fileToLoad = L"C:\\Users\\dipao\\source\\repos\\Phoenix-Engine\\.workspace\\assets\\baked\\Sponza.phxpak";
		m_pakFileTest = std::make_unique<PakFile_Test>(fileToLoad);
		m_pakFileTest->StartMetadataLoad();
	}

	void Shutdown() override
	{
		PHX_INFO("Sandbox app is starting up");
		m_imguiRenderer.Finialize();
	}

	void Tick() override
	{
		m_imguiRenderer.BeginFrame();

		{
			ImGui::Begin("Pak File Manager");
			const uint8_t status = m_pakFileTest->GetStatus();
			switch (status)
			{
			case PakStatus::Unloaded:
				ImGui::Text("Unloaded");
				break;
			case PakStatus::LoadingHeader:
				ImGui::Text("Loaded header...");
				break;
			case PakStatus::LoadingAssetHeaders:
				ImGui::Text("Loaded asset headers...");
				break;

			case::PakStatus::Loaded:
			default:
				for (const PakFileFormat::AssetEntry& entry : m_pakFileTest->GetEntries())
				{
					char buffer[9]; // 8 characters + null terminator
					std::snprintf(buffer, sizeof(buffer), "%08X", entry.FileNameHash);

					if (ImGui::TreeNode(buffer))
					{
						ImGui::Text("ID: %d", entry.FileNameHash);
						ImGui::Text("Uncompressed Size: %d", entry.);
						ImGui::Text("Compressed Size: %d", entry.CompressedSize);
						ImGui::Text("Offset: %lld", entry.AssetHeader.of);

						ImGui::TreePop();
					}
				}
			}

			ImGui::End();
		}

		rhi::CommandCtx* ctx = rhi::BeginCommnadCtx();
		ctx->RenderPassBegin();
		m_imguiRenderer.Render(ctx);
		ctx->RenderPassEnd();

		phx::rhi::Present();
	}

	const char* GetName() const override { return this->m_desc.Name.c_str(); }
	void GetDefaultWindowSize(uint32_t& outWidth, uint32_t& outHeight) const override
	{
		outWidth = m_desc.Width;
		outHeight = m_desc.Height;
	}

	void SetWindowHandle(void* handle) override { m_windowHandle = handle; }
	void* GetWindowHandle() const override { return m_windowHandle; }

private:
	std::unique_ptr<PakFile_Test> m_pakFileTest;

private:
	inline static Sandbox* ms_instance = nullptr;

	
private:
	std::unique_ptr<phx::IRootFileSystem> m_fs;
	const phx::ApplicationDescriptor m_desc;
	phx::gfx::ImGuiRenderSystem m_imguiRenderer;

	void* m_windowHandle;
};

phx::IApplication* phx::CreateApplication()
{
	ApplicationDescriptor desc = {
		.Name = "Sandbox",
		.WorkingDirectory = phx::VFS::GetDirectoryWithExecutable()
	};

	return new Sandbox(desc);
}
