#include <Phoenix.h>
#include <phx/core/EntryPoint.h>

#include <phx/core/VFS.h>
#include <phx/renderer/ImGuiRenderer.h>

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

		m_imguiRenderer.Initialize(m_fs.get(), m_windowHandle);
		m_imguiRenderer.EnableDarkThemeColours();
	}

	void Shutdown() override
	{
		PHX_INFO("Sandbox app is starting up");
		m_imguiRenderer.Finialize();
	}

	void Tick() override
	{
		m_imguiRenderer.BeginFrame();

		rhi::CommandCtx* ctx = rhi::BeingContext();
		m_imguiRenderer.Render(ctx);

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
