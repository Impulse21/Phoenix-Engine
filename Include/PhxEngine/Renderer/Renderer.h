#pragma once

#include <string>
#include <atomic>

#include <PhxEngine/RHI/PhxRHI.h>
#include <PhxEngine/RHI/Dx12/RootSignatureBuilder.h>

#include <PhxEngine/Renderer/SceneComponents.h>

namespace PhxEngine::Renderer
{
	class Scene;

	namespace New
	{
		class Scene;
	}

	enum class RenderPass
	{
		ZPrePass,
		Shadow,
		LighingPass
	};

	struct CullResults
	{
		std::vector<uint32_t> VisibleMeshInstance;
	};

	struct CullParameters
	{
		enum Flags : uint8_t
		{
			kEmpty = 0
		};

		uint32_t Flags = kEmpty;


	};

	extern void FillBindlessDescriptorTable(RHI::IGraphicsDevice* graphicsDevice, RHI::Dx12::DescriptorTable& table);

	class RenderSystem
	{
	public:
		RenderSystem(RHI::IGraphicsDevice& graphicsDevice)
			: m_graphicsDevice(graphicsDevice)
		{};

		void Initialize(std::string const& shaderBinaryPath, std::string const& assetsPath);

		void DrawScene(
			Scene const& scene,
			CullResults const& cullResults,
			RenderPass renderPass, 
			RHI::CommandListHandle commandList);

		void DrawSky();

		void FrustrumCull(
			CameraComponent const& cameraComponent,
			New::Scene const& scene,
			CullResults& results);

		RHI::ShaderHandle LoadShader(RHI::EShaderType shaderType, std::string const& filename);

		const RHI::Dx12::DescriptorTable& GetBindlessDescriptorTable() { return this->m_bindlessDescriptorTable; }
		RHI::IGraphicsDevice& GetGDevice() { return this->m_graphicsDevice; }

	private:
		void LoadShader();
		void InitializeBindlessDescriptorTable();

	private:
		RHI::IGraphicsDevice& m_graphicsDevice;
		RHI::Dx12::DescriptorTable m_bindlessDescriptorTable;
	};
}

