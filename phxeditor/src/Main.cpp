//
// Main.cpp
//

#include "pch.h"

#include "phx/core/CommandLineArgs.h"
#include "phx/core/StringHash.h"
#include "phx/core/Log.h"
#include "phx/core/VFS.h"
#include "phx/core/StringUtils.h"
#include "phx/core/SystemTime.h"

#include "phx/renderer/ImGuiRenderer.h"

#include "phx/rhi/GfxDevice.h"
#include "phx/rhi/ShaderCompiler.h"
#include "phx/rhi/CommandCtx.h"

#include "phx/EngineCore.h"

#include <cmath>
#include <unordered_map>

using namespace phx;


class PhxEditor final : public phx::IEngineApp
{
public:
	void Startup() override 
	{

		m_fs = phx::FileSystemFactory::CreateRootFileSystem();
		phx::FS::RootPtr = m_fs.get();

		std::string projectDir;
		bool hasProjectPath = false;
		{
			std::wstring projectDirW;
			hasProjectPath = phx::CommandLineArgs::GetString(L"project_dir", projectDirW);
			StringConvert(projectDirW, projectDir);
		}

		std::filesystem::path projectDirPath = projectDir;
		if (!hasProjectPath)
		{
			PHX_WARN("No project_dir defined, defaulting to working directory");
			projectDirPath = phx::FS::GetDirectoryWithExecutable();
		}

		std::filesystem::path applicationShaderPath = phx::FS::GetDirectoryWithExecutable() / "shaders/application";
		std::filesystem::path frameworkShaderPath = phx::FS::GetDirectoryWithExecutable() / "shaders/engine";
		std::filesystem::path assetsPath = projectDirPath / "assets";
		std::filesystem::path assetsCachePath = projectDirPath / "assets/.cache";

		m_fs->Mount("/native", phx::FileSystemFactory::CreateNativeFileSystem());
		m_fs->Mount("/shaders", applicationShaderPath);
		m_fs->Mount("/shaders_engine", frameworkShaderPath);
		m_fs->Mount("/assets", assetsPath);
		m_fs->Mount("/assets_cache", assetsCachePath);

		phx::rhi::GfxDevice* device = phx::rhi::GfxDevice::Ptr;
		this->m_imguiRenderer.Initialize(device, m_fs.get());
		this->m_imguiRenderer.EnableDarkThemeColours();
	};

	void Shutdown() override 
	{
		phx::rhi::GfxDevice* device = phx::rhi::GfxDevice::Ptr;
		m_imguiRenderer.Finialize(device);

	};

	void CacheRenderData() override {};
	void Update() override 
	{
		m_imguiRenderer.BeginFrame();
		ImGui::ShowDemoWindow();
	};

	void Render() override
	{
		// phx::rhi::GfxDevice* device = phx::rhi::GfxDevice::Ptr;
		rhi::CommandCtx ctx = {};

		ctx.RenderPassBegin();

		m_imguiRenderer.Render(ctx);

		ctx.RenderPassEnd();
	}

private:
	std::unique_ptr<phx::IRootFileSystem> m_fs;
	gfx::ImGuiRenderSystem m_imguiRenderer;
};

CREATE_APPLICATION(PhxEditor)