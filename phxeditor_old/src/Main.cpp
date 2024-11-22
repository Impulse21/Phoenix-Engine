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

// Function to update the vertex colors based on time
void UpdateTriangleColors(std::array<float, 3>& colorV1,
	std::array<float, 3>& colorV2,
	std::array<float, 3>& colorV3)
{
	float time = phx::SystemTime::GetCurrentTick();  // Get the current time
	constexpr float speed = 0.00001f;  // Slow down the color transitions

	// Use sin and cos functions with a lower frequency to smooth color transitions
	colorV1[0] = (sin(time * speed) + 1.0f) * 0.5f; // R for vertex 1
	colorV1[1] = (cos(time * speed * 0.8f) + 1.0f) * 0.5f; // G for vertex 1
	colorV1[2] = (sin(time * speed * 1.2f) + 1.0f) * 0.5f; // B for vertex 1

	colorV2[0] = (sin(time * speed * 1.3f) + 1.0f) * 0.5f; // R for vertex 2
	colorV2[1] = (cos(time * speed * 0.7f) + 1.0f) * 0.5f; // G for vertex 2
	colorV2[2] = (sin(time * speed * 1.1f) + 1.0f) * 0.5f; // B for vertex 2

	colorV3[0] = (sin(time * speed * 0.9f) + 1.0f) * 0.5f; // R for vertex 3
	colorV3[1] = (cos(time * speed * 1.5f) + 1.0f) * 0.5f; // G for vertex 3
	colorV3[2] = (sin(time * speed * 1.4f) + 1.0f) * 0.5f; // B for vertex 3
}

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

		// Try to load asset
		phx::rhi::GfxDevice* device = phx::rhi::GfxDevice::Ptr;

		phx::rhi::ShaderCompiler::Output testShaderVSOutput = phx::rhi::ShaderCompiler::Compile({
				.Format = device->GetShaderFormat(),
				.ShaderStage = phx::rhi::ShaderStage::VS,
				.SourceFilename = "/shaders/TestShader.hlsl",
				.EntryPoint = "MainVS",
				.FileSystem = m_fs.get()});

		phx::rhi::ShaderCompiler::Output testShaderPSOutput = phx::rhi::ShaderCompiler::Compile({
				.Format = device->GetShaderFormat(),
				.ShaderStage = phx::rhi::ShaderStage::PS,
				.SourceFilename = "/shaders/TestShader.hlsl",
				.EntryPoint = "MainPS",
				.FileSystem = m_fs.get() });

		
		m_pipeline = device->CreatePipeline({
			.VS = {.ByteCode = testShaderVSOutput.ByteCode, .EntryPoint = "MainVS"},
			.PS = {.ByteCode = testShaderPSOutput.ByteCode, .EntryPoint = "MainPS"},
			.DepthStencilState = { .DepthEnable = false, .DepthWriteMask = phx::rhi::DepthWriteMask::Zero },
			.RasterState = {.CullMode = phx::rhi::RasterCullMode::None },
			.VertexBufferBindings = {
				{ .SemanticName = "POSITION", .Format = phx::rhi::Format::RG32_FLOAT },
				{ .SemanticName = "COLOR", .Format = phx::rhi::Format::RGB32_FLOAT },
			},
			.RenderPassInfo = {
				.RTFormats = { phx::rhi::Format::R10G10B10A2_UNORM }
}
			});

		// this->m_imguiRenderSystem.Initialize(device, m_fs.get());
		// this->m_imguiRenderSystem.EnableDarkThemeColours();
	};

	void Shutdown() override 
	{
		phx::rhi::GfxDevice* device = phx::rhi::GfxDevice::Ptr;

		device->DeletePipeline(this->m_pipeline);
		// m_imguiRenderSystem.Finialize(device);

		phx::FS::RootPtr = nullptr;
	};

	void CacheRenderData() override {};
	void Update() override 
	{
		// m_imguiRenderSystem.BeginFrame();
		// ImGui::ShowDemoWindow();
		// phx::EngineProfile::DrawUI();
	};

	void Render() override
	{
		using namespace phx::rhi;
		phx::rhi::GfxDevice* device = phx::rhi::GfxDevice::Ptr;
		// CommandCtx& ctx = device->BeginCommandCtx();
		CommandCtx ctx = {};

		ctx.RenderPassBegin();

		Viewport v(2000, 1700);
		ctx.SetViewports({ v });

		Rect scissor(2000, 1700);
		ctx.SetScissors({ scissor });
		ctx.SetPipelineState(m_pipeline);
#if false
		EmberGfx::DynamicAllocator dynamicAllocator = {};
		EmberGfx::DynamicBuffer dynamicBuffer = dynamicAllocator.Allocate(sizeof(uint16_t) * 3, 16);

		uint16_t* indices = reinterpret_cast<uint16_t*>(dynamicBuffer.Data);
		indices[0] = 0;
		indices[1] = 1;
		indices[2] = 2;

		ctx.SetDynamicIndexBuffer(dynamicBuffer.BufferHandle, dynamicBuffer.Offset, 3, Format::R16_UINT);

		struct Vertex
		{
			std::array<float, 2> Position;
			std::array<float, 3> Colour;
		};

		const size_t bufferSize = sizeof(Vertex) * 3;
		dynamicBuffer = dynamicAllocator.Allocate(bufferSize, 16);
		Vertex* vertices = reinterpret_cast<Vertex*>(dynamicBuffer.Data);
		vertices[0].Position = { 0.0f, 0.5f };
		vertices[1].Position = { 0.5f, -0.5f };
		vertices[2].Position = { -0.5f, -0.5f };

#if false
		UpdateTriangleColors(
			vertices[0].Colour,
			vertices[1].Colour,
			vertices[2].Colour);
#else
		vertices[0].Colour = { 1.0f, 0.0f, 0.0f };
		vertices[1].Colour = { 0.0f, 1.0f, 0.0f };
		vertices[2].Colour = { 0.0f, 0.0f, 1.0f };
#endif
		ctx.SetDynamicVertexBuffer(dynamicBuffer.BufferHandle, dynamicBuffer.Offset, 0, 3, sizeof(Vertex));
		ctx.DrawIndexed(3);

		// m_imguiRenderSystem.Render(ctx);
#endif
		ctx.RenderPassEnd();
	}

private:
	phx::rhi::PipelineStateHandle m_pipeline;
	std::unique_ptr<phx::IRootFileSystem> m_fs;
};

CREATE_APPLICATION(PhxEditor)