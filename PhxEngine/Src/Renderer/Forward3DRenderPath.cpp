#include "C:/Users/dipao/source/repos/Impulse21/Phoenix-Engine/Build/PhxEngine/CMakeFiles/PhxEngine.dir/Debug/cmake_pch.hxx"
#include <PhxEngine/Renderer/Forward3DRenderPath.h>
#include <PhxEngine/Renderer/CommonPasses.h>
#include <PhxEngine/Graphics/ShaderFactory.h>
#include <taskflow/taskflow.hpp>


using namespace PhxEngine;
using namespace PhxEngine::RHI;
using namespace PhxEngine::Graphics;
using namespace PhxEngine::Renderer;

PhxEngine::Renderer::Forward3DRenderPath::Forward3DRenderPath(
	RHI::IGraphicsDevice* gfxDevice,
	std::shared_ptr<Renderer::CommonPasses> commonPasses,
	std::shared_ptr<Graphics::ShaderFactory> shaderFactory)
	: m_gfxDevice(gfxDevice)
	, m_commonPasses(commonPasses)
	, m_shaderFactory(shaderFactory)
{
}

void PhxEngine::Renderer::Forward3DRenderPath::Initialize()
{
	tf::Executor executor;
	tf::Taskflow taskflow;

	tf::Task shaderLoadTask = this->LoadShaders(taskflow);
	tf::Task createPipelineStates = this->LoadPipelineStates(taskflow);

	shaderLoadTask.precede(createPipelineStates);  // A runs before B and C


	executor.run(taskflow).wait();
}

tf::Task PhxEngine::Renderer::Forward3DRenderPath::LoadShaders(tf::Taskflow& taskflow)
{
	tf::Task shaderLoadTask = taskflow.emplace([this](tf::Subflow& subflow){
		// Load Shaders
		subflow.emplace([]() {});
		});

	return shaderLoadTask;
}

tf::Task PhxEngine::Renderer::Forward3DRenderPath::LoadPipelineStates(tf::Taskflow& taskflow)
{
	tf::Task createPipelineStatesTask = taskflow.emplace([this](tf::Subflow& subflow) {
		// Load Shaders
		subflow.emplace([]() {});
		});

	return createPipelineStatesTask;
}
