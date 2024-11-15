//
// Main.cpp
//

#include "pch.h"

#include "phx/core/CommandLineArgs.h"

#include "phx/core/Log.h"
#include "phx/core/VFS.h"
#include "phx/core/StringUtils.h"
#include "phx/core/SystemTime.h"

#include "phx/rhi/GfxDevice.h"
#include "phx/rhi/ShaderCompiler.h"

#include "phx/EngineCore.h"

#include <cmath>

using namespace phx;

class PhxEditor final : public phx::IEngineApp
{
public:
	void Startup() override 
	{
		phx::rhi::GfxDevice* device = phx::rhi::GfxDevice::Ptr;
	};

	void Shutdown() override 
	{
		phx::rhi::GfxDevice* device = phx::rhi::GfxDevice::Ptr;

	};

	void CacheRenderData() override {};
	void Update() override 
	{
	};

	void Render() override
	{
	}

private:
	phx::rhi::PipelineStateHandle m_pipeline;
	std::unique_ptr<phx::IRootFileSystem> m_fs;
};

CREATE_APPLICATION(PhxEditor)