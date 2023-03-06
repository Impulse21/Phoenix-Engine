#include "C:/Users/dipao/source/repos/Impulse21/Phoenix-Engine/Build/PhxEngine/CMakeFiles/PhxEngine.dir/Debug/cmake_pch.hxx"
#include "DDGIPass.h"
#include <taskflow/taskflow.hpp>
#include <PhxEngine/Graphics/ShaderFactory.h>
#include <PhxEngine/Scene/Scene.h>

using namespace PhxEngine::Graphics;
using namespace PhxEngine::Renderer;
using namespace PhxEngine::RHI;
using namespace DirectX;

void DDGI::Initialize(tf::Taskflow& taskflow, ShaderFactory& shaderFactory)
{
	// TODO: Create shaders and Pipeline statess
}

void PhxEngine::Renderer::DDGI::Render(RHI::ICommandList* commandList, Scene::Scene& scene)
{
	{
		commandList->BeginScopedMarker("DDGI: Ray Trace");
		Shader::RayTracePushConstants push = {};
		push.RandomOrientation = ;
		push.NumFrames = ;
		push.InfiniteBounces = 1;
		push.GiIntensity = 1.7;

		commandList->BindPushConstant(0, push);

	}
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

	if (this->m_sampleProbeGrid.IsValid())
	{
		this->m_gfxDevice->DeleteTexture(this->m_sampleProbeGrid);
	}

	this->m_sampleProbeGrid = this->m_gfxDevice->CreateTexture({
			.BindingFlags = BindingFlags::UnorderedAccess | BindingFlags::ShaderResource,
			.Dimension = RHI::TextureDimension::Texture2D,
			.InitialState = RHI::ResourceStates::UnorderedAccess,
			.Format = RHI::RHIFormat::RGBA16_FLOAT,
			.Width = (uint32_t)this->m_canvas.x,
			.Height = (uint32_t)this->m_canvas.y,
			.MipLevels = 1,
			.DebugName = "DDGI::SampleProbeGrid",
		});

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

