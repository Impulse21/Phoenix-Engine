#pragma once

#include <PhxEngine/RHI/PhxRHI.h>
#include <DirectXMath.h>

namespace PhxEngine::Graphics
{
	class ShaderFactory;
}

namespace PhxEngine::Scene
{
	class Scene;
}

namespace tf
{
	class Task;
	class Taskflow;
}

namespace PhxEngine::Renderer
{
	class DDGI
	{
	public:
		void Initialize(tf::Taskflow& taskflow, Graphics::ShaderFactory& shaderFactory);

		void Render(RHI::ICommandList* commandList, Scene::Scene& scene);

		void BuidUI();

		struct Settings
		{
			bool OverrideProbeGridDimensions = false;
		};

		Settings& GetSettings() { return this->m_settings; }

	private:
		void InitializeProbeGrid();

	private: 
		Settings m_settings;
		uint32_t m_sceneId;

		struct ProbeGrid
		{
			float ProbeDistance = 1.0f;
			DirectX::XMFLOAT3 GridStartPosition;
			DirectX::XMINT3 ProbeCount;
		} m_probeGrid;
	};
}

