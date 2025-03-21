#pragma once

#include <PhxRenderer/RenderSystem.h>

#include <vector>
#include <memory>

namespace phx::gfx
{
	namespace ForwardRenderPasses
	{
		enum
		{
			GeometryPass = 0,
			ShadowPass,
			GuiPass,
		};
	}
	class ForwardRenderer final
	{
	public:
		ForwardRenderer() = default;
		~ForwardRenderer() = default;

		template<typename TSystem>
		void RegisterRenderSystem(uint32_t passMask)
		{
			m_renderSystems.emplace_back(std::make_unique<TSystem>());
			m_passMasks.push_back(passMask);
			m_cachedData.push_back(nullptr);
		}

	private:
		std::vector<std::unique_ptr<IRenderSystem>> m_renderSystems;
		std::vector<uint32_t> m_passMasks;
		std::vector<void*> m_cachedData;
	};

}

