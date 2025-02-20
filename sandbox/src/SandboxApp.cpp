#include <Phoenix.h>
#include <mutex>
#include <atomic>

#include "PhxCore/EntryPoint.h"
#include "PhxCore/StringHash.h"
#include "PhxCore/VFS.h"
#include "Generated/GlobalVariables.h"

#include "PhxRenderer/ImGuiRenderer.h"

#include "PhxRenderer/MeshResourceFactory.h"
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

		m_imguiRenderer.Initialize(m_fs.get(), m_windowHandle);
		m_imguiRenderer.EnableDarkThemeColours();

		phx::ResourceManger::Initialize(GlobalPaths::AssetsDirectory);
		phx::ResourceManger::MountPak("res:/Sponza.phxpak");

		phx::ResourceManger::RegisterFactory<phx::renderer::MeshResourceFactory>();
		RefCountPtr<IResource> meshResource = phx::ResourceManger::Get("lionhead", ".phxmsh");
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
