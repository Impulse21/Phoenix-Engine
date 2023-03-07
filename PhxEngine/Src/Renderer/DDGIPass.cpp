#include "C:/Users/dipao/source/repos/Impulse21/Phoenix-Engine/Build/PhxEngine/CMakeFiles/PhxEngine.dir/Debug/cmake_pch.hxx"
#include "DDGIPass.h"
#include <taskflow/taskflow.hpp>
#include <PhxEngine/Graphics/ShaderFactory.h>
#include <PhxEngine/Scene/Scene.h>
#include <PhxEngine/Shaders/ShaderInteropStructures.h>
#include <PhxEngine/Core/Helpers.h>

using namespace PhxEngine::Graphics;
using namespace PhxEngine::Renderer;
using namespace PhxEngine::RHI;
using namespace DirectX;

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

