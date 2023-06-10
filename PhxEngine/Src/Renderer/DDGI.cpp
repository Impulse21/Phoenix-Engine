#include "C:/Users/dipao/source/repos/Impulse21/Phoenix-Engine/Build/PhxEngine/CMakeFiles/PhxEngine.dir/Debug/cmake_pch.hxx"

#include <taskflow/taskflow.hpp>
#include <PhxEngine/Graphics/ShaderFactory.h>
#include <PhxEngine/Scene/Scene.h>
#include <PhxEngine/Shaders/ShaderInteropStructures.h>
#include <PhxEngine/Core/Helpers.h>
#include <PhxEngine/Renderer/DDGI.h>
#include <imgui.h>

using namespace PhxEngine::Graphics;
using namespace PhxEngine::Renderer;
using namespace PhxEngine::RHI;
using namespace DirectX;


void PhxEngine::Renderer::DDGI::UpdateResources(RHI::IGraphicsDevice* gfxDevice)
{
	// Construct data
	uint32_t width = this->RayCount;
	uint32_t height = this->GetProbeCount();

	// -- Create RT Output texture that stored the Radiance data ---
	if (!this->RTRadianceOutput.IsValid() || 
		gfxDevice->GetTextureDesc(this->RTRadianceOutput).Width < width ||
		gfxDevice->GetTextureDesc(this->RTRadianceOutput).Height < height)
	{
		if (this->RTRadianceOutput.IsValid())
		{
			gfxDevice->DeleteTexture(this->RTRadianceOutput);
		}

		this->RTRadianceOutput = gfxDevice->CreateTexture({
			.BindingFlags = BindingFlags::UnorderedAccess | BindingFlags::ShaderResource,
			.Dimension = RHI::TextureDimension::Texture2D,
			.InitialState = RHI::ResourceStates::UnorderedAccess,
			.Format = RHI::RHIFormat::RGBA16_FLOAT,
			.Width = width,
			.Height = height,
			.MipLevels = 1,
			.DebugName = "RTRadianceOutput",
			});
	}

	// -- Create RT Output texture that stored the Distance and depth data ---
	if (!this->RTDirectionDepthOutput.IsValid() ||
		gfxDevice->GetTextureDesc(this->RTDirectionDepthOutput).Width < width ||
		gfxDevice->GetTextureDesc(this->RTDirectionDepthOutput).Height < height)
	{
		if (this->RTDirectionDepthOutput.IsValid())
		{
			gfxDevice->DeleteTexture(this->RTRadianceOutput);
		}

		this->RTDirectionDepthOutput = gfxDevice->CreateTexture({
			.BindingFlags = BindingFlags::UnorderedAccess | BindingFlags::ShaderResource,
			.Dimension = RHI::TextureDimension::Texture2D,
			.InitialState = RHI::ResourceStates::UnorderedAccess,
			.Format = RHI::RHIFormat::RGBA16_FLOAT,
			.Width = width,
			.Height = height,
			.MipLevels = 1,
			.DebugName = "RTDirectionDepthOutput",
			});
	}

	// -- Create the probe data ---
	// Add 2 pixel border to allow for bilinear interpolation
	const uint32_t octahedralIrradianceSize = Shader::New::DDGI_COLOUR_TEXELS;
	width = octahedralIrradianceSize * this->GridDimensions.x * this->GridDimensions.y;
	height = octahedralIrradianceSize * this->GridDimensions.z;

	// -- Create RT Output texture that stored the Radiance data ---
	if (!this->ProbeIrradianceAtlas[0].IsValid() ||
		gfxDevice->GetTextureDesc(this->ProbeIrradianceAtlas[0]).Width < width ||
		gfxDevice->GetTextureDesc(this->ProbeIrradianceAtlas[0]).Height < height)
	{
		for (int i = 0; i < this->ProbeIrradianceAtlas.size(); i++)
		{
			if (this->ProbeIrradianceAtlas[i].IsValid())
			{
				gfxDevice->DeleteTexture(this->ProbeIrradianceAtlas[i]);
			}

			this->ProbeIrradianceAtlas[i] = gfxDevice->CreateTexture({
				.BindingFlags = BindingFlags::UnorderedAccess | BindingFlags::ShaderResource,
				.Dimension = RHI::TextureDimension::Texture2D,
				.InitialState = RHI::ResourceStates::ShaderResource,
				.Format = RHI::RHIFormat::RGBA16_FLOAT,
				.Width = width,
				.Height = height,
				.MipLevels = 1,
				.DebugName = "Probe Atlas (Irradiance)",
				});
		}
	}

	// Visibility Texture
	const uint32_t octahedralVisibilitySize = Shader::New::DDGI_DEPTH_TEXELS;
	width = octahedralVisibilitySize * this->GridDimensions.x * this->GridDimensions.y;
	height = octahedralVisibilitySize * this->GridDimensions.z;

	// -- Create RT Output texture that stored the Radiance data ---
	if (!this->ProbeVisibilityAtlas[0].IsValid() ||
		gfxDevice->GetTextureDesc(this->ProbeVisibilityAtlas[0]).Width < width ||
		gfxDevice->GetTextureDesc(this->ProbeVisibilityAtlas[0]).Height < height)
	{
		for (int i = 0; i < this->ProbeVisibilityAtlas.size(); i++)
		{
			if (this->ProbeVisibilityAtlas[i].IsValid())
			{
				gfxDevice->DeleteTexture(this->ProbeVisibilityAtlas[i]);
			}

			this->ProbeVisibilityAtlas[i] = gfxDevice->CreateTexture({
				.BindingFlags = BindingFlags::UnorderedAccess | BindingFlags::ShaderResource,
				.Dimension = RHI::TextureDimension::Texture2D,
				.InitialState = RHI::ResourceStates::ShaderResource,
				.Format = RHI::RHIFormat::RG16_FLOAT,
				.Width = width,
				.Height = height,
				.MipLevels = 1,
				.DebugName = "Probe Atlas (Visibility)",
				});
		}
	}

	width = this->GridDimensions.x * this->GridDimensions.y;
	height = this->GridDimensions.z;

	// -- Create RT Output texture that stored the Radiance data ---
	if (!this->ProbeOffsets.IsValid() ||
		gfxDevice->GetTextureDesc(this->ProbeOffsets).Width < width ||
		gfxDevice->GetTextureDesc(this->ProbeOffsets).Height < height)
	{
		if (this->ProbeOffsets.IsValid())
		{
			gfxDevice->DeleteTexture(this->ProbeOffsets);
		}

		this->ProbeOffsets = gfxDevice->CreateTexture({
			.BindingFlags = BindingFlags::UnorderedAccess | BindingFlags::ShaderResource,
			.Dimension = RHI::TextureDimension::Texture2D,
			.InitialState = RHI::ResourceStates::ShaderResource,
			.Format = RHI::RHIFormat::RGBA16_FLOAT,
			.Width = width,
			.Height = height,
			.MipLevels = 1,
			.DebugName = "Probe Offsets",
			});
	}
}

void PhxEngine::Renderer::DDGI::BuildUI()
{
	ImGui::InputInt3("Grid Dimensions", reinterpret_cast<int32_t*>(&this->GridDimensions.x));
	ImGui::InputInt("Ray Count", &this->RayCount);
	ImGui::Text("Grid Min: (%.6f, %.6f. %.6f)", this->GridMin.x, this->GridMin.y, this->GridMin.z);
	ImGui::Text("Grid Max: (% .6f, % .6f. % .6f", this->GridMax.x, this->GridMax.y, this->GridMax.z);
	ImGui::Text("Irradiance OctSize: %d", this->IrradianceOctSize);
	ImGui::Text("Depth OctSize: %d", this->DepthOctSize);
}						   

#if false
void DDGI::Initialize(tf::Taskflow& taskflow, ShaderFactory& shaderFactory)
{
	this->m_rayTraceComputeShader = shaderFactory.CreateShader(
		"PhxEngine/DDGIRayTraceCS.hlsl",
		{
			.Stage = ShaderStage::Compute,
			.DebugName = "DDGIRayTraceCS"
		});

	this->m_rayTraceComputePipeline = this->m_gfxDevice->CreateComputePipeline({
			.ComputeShader = this->m_rayTraceComputeShader
		});

}

void PhxEngine::Renderer::DDGI::Render(RHI::ICommandList* commandList, Scene::Scene& scene)
{
	{
		commandList->BeginScopedMarker("DDGI: Ray Trace");
		Shader::DDGI::PushConstants push = {};
		push.InstanceInclusionMask = 0xFF;
		push.FrameIndex = this->m_gfxDevice->GetFrameIndex();
		push.RayCount = this->m_probeGrid.RaysPerProbe;
		push.BlendSpeed = 0.02f;

		commandList->SetComputeState(this->m_rayTraceComputePipeline);
		commandList->BindPushConstant(0, push);

		Shader::MiscPushConstants cb = {};
		float angle = Core::Random::GetRandom(0.0f, 1.0f) * DirectX::XM_2PI;
		XMVECTOR axis = XMVectorSet(
			Core::Random::GetRandom(-1.0f, 1.0f),
			Core::Random::GetRandom(-1.0f, 1.0f),
			Core::Random::GetRandom(-1.0f, 1.0f),
			0);

		axis = DirectX::XMVector3Normalize(axis);
		DirectX::XMStoreFloat4x4(&cb.Transform, DirectX::XMMatrixRotationAxis(axis, angle));

		commandList->BindDynamicConstantBuffer(3, cb);
		commandList->BindDynamicUavDescriptorTable(4, { this->m_rayBuffer });
		const uint32_t totalProbes = this->m_probeGrid.ProbeCount.x * this->m_probeGrid.ProbeCount.y * this->m_probeGrid.ProbeCount.z;

		commandList->Dispatch(totalProbes, 1, 1);
	}

	{
		RHI::GpuBarrier barriers[] =
		{
			RHI::GpuBarrier::CreateMemory(),
			RHI::GpuBarrier::CreateBuffer(this->m_rayBuffer, IGraphicsDevice::GPtr->GetBufferDesc(this->m_rayBuffer).InitialState, RHI::ResourceStates::ShaderResourceNonPixel),
			RHI::GpuBarrier::CreateTexture(this->m_depthTextures[1], IGraphicsDevice::GPtr->GetTextureDesc(this->m_depthTextures[1]).InitialState, RHI::ResourceStates::UnorderedAccess),
			// TODO Swich to an offset buffer.
			RHI::GpuBarrier::CreateBuffer(this->m_rayBuffer, IGraphicsDevice::GPtr->GetBufferDesc(this->m_rayBuffer).InitialState, RHI::ResourceStates::UnorderedAccess),
		};

		commandList->TransitionBarriers(Core::Span<RHI::GpuBarrier>(barriers, _countof(barriers)));
	}
}

void PhxEngine::Renderer::DDGI::BuidUI()
{
}

void PhxEngine::Renderer::DDGI::InitializeProbeGrid(Scene::Scene const& scene)
{
	DirectX::XMVECTOR minExtentsV = DirectX::XMLoadFloat3(&scene.GetBoundingBox().Min);
	DirectX::XMVECTOR maxExtentsV = DirectX::XMLoadFloat3(&scene.GetBoundingBox().Min);

	DirectX::XMVECTOR sceneLength = maxExtentsV - minExtentsV;

	// Compute the number of probes
	// Add two extra probs per dimension to ensure the scene is covered.
	DirectX::XMVECTOR probeCount = (sceneLength / this->m_probeGrid.ProbeDistance) + XMVectorSet(2.0f, 2.0f, 2.0f, 0.0f);
	DirectX::XMStoreInt3((uint32_t*)&this->m_probeGrid.ProbeCount.x, probeCount);

	this->m_probeGrid.GridStartPosition = scene.GetBoundingBox().Min;
	this->m_probeGrid.ProbeMaxDistance = this->m_probeGrid.ProbeMaxDistance * 1.5f;

	this->RecreateProbeGrideResources();

}

void PhxEngine::Renderer::DDGI::RecreateProbeGrideResources()
{
	const uint32_t totalProbes = this->m_probeGrid.ProbeCount.x * this->m_probeGrid.ProbeCount.y * this->m_probeGrid.ProbeCount.z;
	const uint32_t bufferCount = this->m_gfxDevice->GetViewportDesc().BufferCount;

	// Ray Tracing
	if (this->m_radianceTexture.IsValid())
	{
		this->m_gfxDevice->DeleteTexture(this->m_radianceTexture);
	}

	this->m_radianceTexture = this->m_gfxDevice->CreateTexture({
			.BindingFlags = BindingFlags::UnorderedAccess | BindingFlags::ShaderResource,
			.Dimension = RHI::TextureDimension::Texture2D,
			.InitialState = RHI::ResourceStates::UnorderedAccess,
			.Format = RHI::RHIFormat::RGBA16_FLOAT,
			.Width = this->m_probeGrid.RaysPerProbe,
			.Height = totalProbes,
			.MipLevels = 1,
			.DebugName = "DDGI::RadianceImage",
		});

	if (this->m_depthTexture.IsValid())
	{
		this->m_gfxDevice->DeleteTexture(this->m_depthTexture);
	}

	this->m_depthTexture = this->m_gfxDevice->CreateTexture({
			.BindingFlags = BindingFlags::UnorderedAccess | BindingFlags::ShaderResource,
			.Dimension = RHI::TextureDimension::Texture2D,
			.InitialState = RHI::ResourceStates::UnorderedAccess,
			.Format = RHI::RHIFormat::RGBA16_FLOAT,
			.Width = this->m_probeGrid.RaysPerProbe,
			.Height = totalProbes,
			.MipLevels = 1,
			.DebugName = "DDGI::RadianceImage",
		});

	// Probe Grid
	 // 1-pixel of padding surrounding each probe, 1-pixel padding surrounding entire texture for alignment.
	const uint32_t irradianceWidth = (this->m_probeGrid.IrradianceOctSize + 2) * this->m_probeGrid.ProbeCount.x * this->m_probeGrid.ProbeCount.y + 2;
	const uint32_t irradianceHeight = (this->m_probeGrid.IrradianceOctSize + 2) * this->m_probeGrid.ProbeCount.z + 2;

	const uint32_t depthWidth = (this->m_probeGrid.DepthOctSize + 2) * this->m_probeGrid.ProbeCount.x * this->m_probeGrid.ProbeCount.y + 2;
	const uint32_t depthHeight = (this->m_probeGrid.DepthOctSize + 2) * this->m_probeGrid.ProbeCount.z + 2;

	if (this->m_irradianceTextures.empty())
	{
		this->m_irradianceTextures.resize(bufferCount);
	}

	if (this->m_depthTextures.empty())
	{
		this->m_depthTextures.resize(bufferCount);
	}

	for (uint32_t i = 0; i < bufferCount; i++)
	{
		if (this->m_irradianceTextures[i].IsValid())
		{
			this->m_gfxDevice->DeleteTexture(this->m_irradianceTextures[i]);
		}

		this->m_irradianceTextures[i] = this->m_gfxDevice->CreateTexture({
				.BindingFlags = BindingFlags::UnorderedAccess | BindingFlags::ShaderResource,
				.Dimension = RHI::TextureDimension::Texture2D,
				.InitialState = RHI::ResourceStates::UnorderedAccess,
				.Format = RHI::RHIFormat::RGBA16_FLOAT,
				.Width = irradianceWidth,
				.Height = irradianceHeight,
				.MipLevels = 1,
				.DebugName = "DDGI::IrradianceImage",
			});

		if (this->m_depthTextures[i].IsValid())
		{
			this->m_gfxDevice->DeleteTexture(this->m_depthTextures[i]);
		}

		this->m_depthTextures[i] = this->m_gfxDevice->CreateTexture({
				.BindingFlags = BindingFlags::UnorderedAccess | BindingFlags::ShaderResource,
				.Dimension = RHI::TextureDimension::Texture2D,
				.InitialState = RHI::ResourceStates::UnorderedAccess,
				.Format = RHI::RHIFormat::RG16_FLOAT,
				.Width = depthWidth,
				.Height = depthHeight,
				.MipLevels = 1,
				.DebugName = "DDGI::DepthImage",
			});
	}

	// Sample Probe grid

	if (this->m_rayBuffer.IsValid())
	{
		this->m_gfxDevice->DeleteBuffer(this->m_rayBuffer);
	}

	this->m_rayBuffer = this->m_gfxDevice->CreateBuffer({
			.MiscFlags = BufferMiscFlags::Structured,
			.Binding = BindingFlags::UnorderedAccess | BindingFlags::ShaderResource,
			.InitialState = ResourceStates::UnorderedAccess,
			.StrideInBytes = sizeof(Shader::DDGI::PackedRayData),
			.SizeInBytes = totalProbes * this->m_probeGrid.RaysPerProbe * sizeof(Shader::DDGI::PackedRayData),
			.DebugName = "Ray Buffer"
		});

}
#endif
