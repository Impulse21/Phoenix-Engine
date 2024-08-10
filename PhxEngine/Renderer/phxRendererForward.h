#pragma once

#include <memory>
#include <vector>
#include <unordered_map>

#include "Core/phxEnumClassFlags.h"

namespace phx
{
	class IRenderSystem;
	enum class RenderPassFlags
	{
		Geometry,
		Shadow,
		PostProcess,
	};

	class RendererForward final
	{
	public:
		RendererForward() = default;
		~RendererForward() = default;

	public:
		template<class TRenderSystem, class... _Types>
		void RegisterRenderSystem(_Types&&... _Args)
		{
			this->m_renderSystems.push_back(
				std::make_unique<TRenderSystem>(
					std::forward<_Types>(_Args)...));
		}

		void CacheRenderData();
		void Render();

	private:
		std::vector<std::unique_ptr<IRenderSystem>> m_renderSystems;
	};

}