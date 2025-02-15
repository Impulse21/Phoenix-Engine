#include <Phoenix.h>
#include "PhxCore/EntryPoint.h"

#include "PhxCore/VFS.h"
#include "PhxRenderer/ImGuiRenderer.h"


#include <dstorage.h>

using namespace phx;


namespace
{
	enum class StatusArrayEntry : uint32_t
	{
		Metadata,
		CpuData,
		GpuData,
		NumEntries
	};

	Microsoft::WRL::ComPtr<IDStorageQueue1> g_dsSystemMemoryQueue;
	Microsoft::WRL::ComPtr<IDStorageFactory> g_dsFactory;

	constexpr wchar_t* fileToLoad = L"C:\\Users\\dipao\\source\\repos\\Phoenix-Engine\\.workspace\\assets\\baked\\Sponza.phxpak";

	void Test_InitDStorage()
	{
		// Load a file and init D3D12
		HRESULT hr = DStorageGetFactory(IID_PPV_ARGS(&g_dsFactory));
		PHX_ASSERT(SUCCEEDED(hr));
		g_dsFactory->SetDebugFlags(DSTORAGE_DEBUG_BREAK_ON_ERROR | DSTORAGE_DEBUG_SHOW_ERRORS);
		g_dsFactory->SetStagingBufferSize(256 * 1024 * 1024);


		BY_HANDLE_FILE_INFORMATION info{};
		hr = (g_dsFile->GetFileInformation(&info));
		PHX_ASSERT(SUCCEEDED(hr));
		uint32_t fileSize = info.nFileSizeLow;
		PHX_INFO("FileSize {0}.", fileSize);

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

	void Test_CreateFile()
	{
	}

	class PakFile_Test
	{
	public:
		PakFile_Test(std::filesystem::path const& path)
		{
			HRESULT hr = g_dsFactory->OpenFile(path.wstring().c_str(), IID_PPV_ARGS(&m_dsFile));
			if (FAILED(hr))
			{
				std::string outMsg;
				StringConvert(fileToLoad, outMsg);
				PHX_ERROR("The file {0}, could no open.", outMsg);
				return;
			}

			hr = (g_dsFactory->CreateStatusArray(
				static_cast<uint32_t>(StatusArrayEntry::NumEntries),
				nullptr,
				IID_PPV_ARGS(&m_statusArray)));

			PHX_ASSERT(SUCCEEDED(hr));
		}

		~PakFile_Test()
		{
			// All requests created for this instance are tagged with 'this', so we can
			// cancel any outstanding requests.
			g_dsSystemMemoryQueue->CancelRequestsWithTag(0xFFFFFFFFFFFFll, reinterpret_cast<uint64_t>(this));
		}

	private:

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


	}

	void Shutdown() override
	{
		PHX_INFO("Sandbox app is starting up");
		m_imguiRenderer.Finialize();
	}

	void Tick() override
	{
		m_imguiRenderer.BeginFrame();

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
		r.Source.File.Source = m_file.Get();
		r.Source.File.Offset = offset;
		r.Source.File.Size = static_cast<uint32_t>(sizeof(T));
		r.Destination.Memory.Buffer = dest;
		r.Destination.Memory.Size = r.Source.File.Size;
		r.UncompressedSize = r.Destination.Memory.Size;
		r.CancellationTag = reinterpret_cast<uint64_t>(this);

		g_dsSystemMemoryQueue->EnqueueRequest(&r);
	}
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
