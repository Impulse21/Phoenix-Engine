//
// Main.cpp
//

#include "pch.h"

#include "phxEngineCore.h"
#include "EmberGfx/phxEmber.h"
#include "phxEngineProfiler.h"

#include "phxDisplay.h"

#include "EmberGfx/phxImGuiRenderer.h"
#include "EmberGfx/phxShaderCompiler.h"
#include "phxCommandLineArgs.h"

#include "phxVFS.h"
#include "phxSystemTime.h"


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

#include "entt/entt.hpp"
#include "phxMath.h"

using namespace phx;

namespace temp
{
	// Create an asset resource that will be drawn X number of times

	struct Drawable
	{
	public:
		math::Sphere m_boudingSphere;
		math::AABB m_boundingBox;
		uint32_t m_numMeshes;
		
		gfx::BufferHandle m_dataBuffer;
		gfx::BufferHandle m_materialConstants;
		std::vector<gfx::TextureHandle> m_textueHandle;
	};
}

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
		phx::gfx::GpuDevice* device = phx::gfx::EmberGfx::GetDevice();

		phx::gfx::ShaderCompiler::Output testShaderVSOutput = phx::gfx::ShaderCompiler::Compile({
				.Format = device->GetShaderFormat(),
				.ShaderStage = phx::gfx::ShaderStage::VS,
				.SourceFilename = "/shaders/TestShader.hlsl",
				.EntryPoint = "MainVS",
				.FileSystem = m_fs.get()});

		phx::gfx::ShaderHandle vsShader = device->CreateShader({
				.Stage = phx::gfx::ShaderStage::VS,
				.ByteCode = phx::Span(testShaderVSOutput.ByteCode, testShaderVSOutput.ByteCodeSize),
				.EntryPoint = "MainVS"});

		phx::gfx::ShaderCompiler::Output testShaderPSOutput = phx::gfx::ShaderCompiler::Compile({
				.Format = device->GetShaderFormat(),
				.ShaderStage = phx::gfx::ShaderStage::PS,
				.SourceFilename = "/shaders/TestShader.hlsl",
				.EntryPoint = "MainPS",
				.FileSystem = m_fs.get() });

		phx::gfx::ShaderHandle psShader = device->CreateShader({
				.Stage = phx::gfx::ShaderStage::PS,
				.ByteCode = phx::Span(testShaderPSOutput.ByteCode, testShaderPSOutput.ByteCodeSize),
				.EntryPoint = "MainPS"});

		phx::gfx::InputLayout il = {
			.elements = {
				{
					.SemanticName = "POSITION",
					.SemanticIndex = 0,
					.Format = phx::gfx::Format::RG32_FLOAT,
				},
				{
					.SemanticName = "COLOR",
					.SemanticIndex = 1,
					.Format = phx::gfx::Format::RGB32_FLOAT,
				}
			}
		};

		phx::gfx::DepthStencilRenderState dss = { .DepthEnable = false, .DepthWriteMask = phx::gfx::DepthWriteMask::Zero};
		phx::gfx::RasterRenderState rs = {.CullMode = phx::gfx::RasterCullMode::None };

		phx::gfx::RenderPassInfo passInfo;
		passInfo.RenderTargetCount = 1;
		passInfo.RenderTargetFormats[0] = phx::gfx::g_SwapChainFormat;

		this->m_pipeline = device->CreatePipeline({
				.VS = vsShader,
				.PS = psShader,
				.DepthStencilRenderState = &dss,
				.RasterRenderState = &rs,
				.InputLayout = &il
			},
			&passInfo);

		device->DeleteShader(vsShader);
		device->DeleteShader(psShader);

		this->m_imguiRenderSystem.Initialize(device, m_fs.get());
		this->m_imguiRenderSystem.EnableDarkThemeColours();
	};

	void Shutdown() override 
	{
		phx::gfx::GpuDevice* device = phx::gfx::EmberGfx::GetDevice();

		device->DeletePipeline(this->m_pipeline);
		m_imguiRenderSystem.Finialize(device);

		phx::FS::RootPtr = nullptr;
	};

	void CacheRenderData() override {};
	void Update() override 
	{
		PHX_EVENT();
		m_imguiRenderSystem.BeginFrame();
		ImGui::ShowDemoWindow();
		// phx::EngineProfile::DrawUI();
	};

	void Render() override
	{
		using namespace phx::gfx;
		GpuDevice* device = EmberGfx::GetDevice();
		CommandCtx* ctx = device->BeginCommandCtx();

		ctx->RenderPassBegin();

		Viewport v(g_DisplayWidth, g_DisplayHeight);
		ctx->SetViewports({ v });

		Rect scissor(g_DisplayWidth, g_DisplayHeight);
		ctx->SetScissors({ scissor });
		ctx->SetPipelineState(m_pipeline);

		EmberGfx::DynamicAllocator dynamicAllocator = {};
		EmberGfx::DynamicBuffer dynamicBuffer = dynamicAllocator.Allocate(sizeof(uint16_t) * 3, 16);

		uint16_t* indices = reinterpret_cast<uint16_t*>(dynamicBuffer.Data);
		indices[0] = 0;
		indices[1] = 1;
		indices[2] = 2;

		ctx->SetDynamicIndexBuffer(dynamicBuffer.BufferHandle, dynamicBuffer.Offset, 3, Format::R16_UINT);

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
		ctx->SetDynamicVertexBuffer(dynamicBuffer.BufferHandle, dynamicBuffer.Offset, 0, 3, sizeof(Vertex));
		ctx->DrawIndexed(3);

		m_imguiRenderSystem.Render(ctx);

		ctx->RenderPassEnd();
	}

private:
	phx::gfx::PipelineStateHandle m_pipeline;
	phx::gfx::ImGuiRenderSystem m_imguiRenderSystem;
	std::unique_ptr<phx::IRootFileSystem> m_fs;
};

CREATE_APPLICATION(PhxEditor)