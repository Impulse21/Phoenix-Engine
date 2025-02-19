#include <Phoenix.h>
#include <mutex>
#include <atomic>

#include "PhxCore/EntryPoint.h"
#include "PhxCore/StringHash.h"

#include "PhxCore/IO/PakFile.h"

#include "PhxCore/VFS.h"
#include "PhxRenderer/ImGuiRenderer.h"

#include "PhxRenderer/MeshResourceHandler.h"
#include "PhxResource/ResourceManger.h"

using namespace phx;

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

		phx::ResourceManger::RegisterResourceHandler<phx::renderer::MeshResourceHandler>();

		m_imguiRenderer.Initialize(m_fs.get(), m_windowHandle);
		m_imguiRenderer.EnableDarkThemeColours();

		phx::InitDStorage();
		const wchar_t* fileToLoad = L"C:\\Users\\dipao\\source\\repos\\Phoenix-Engine\\.workspace\\assets\\baked\\Sponza.phxpak";
		m_pakFileTest = std::make_unique<PakFile>(fileToLoad);
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
					std::snprintf(buffer, sizeof(buffer), "%08X", entry.Hash);
					
					const char* fileName = m_pakFileTest->FindFilenameByHash(entry.Hash);
					if (ImGui::TreeNode(fileName ? fileName : buffer))
					{
						ImGui::Text("ID: %s", buffer);
						ImGui::Text("Uncompressed Size: %d", entry.Size);
						ImGui::Text("Offset: %lld", entry.Offset);
						ImGui::Text("Num Dependecies: %d", entry.NumDependiences);

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
	std::unique_ptr<PakFile> m_pakFileTest;

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
