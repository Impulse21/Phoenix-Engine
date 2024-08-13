#pragma once

#include <memory>
#include <vector>
#include <unordered_map>
#include <type_traits>

#include "phxRenderSystem.h"
#include "Core/phxEnumClassFlags.h"

namespace phx
{
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
			static_assert(std::is_base_of_v<IRenderSystem, TRenderSystem>());
			this->m_renderSystems.push_back(
				std::make_unique<TRenderSystem>(
					std::forward<_Types>(_Args)...));
		}

		void CacheRenderData();
		void Render();

	private:
		std::vector<std::unique_ptr<IRenderSystem>> m_renderSystems;

		struct Viewport
		{

		};
	};

}