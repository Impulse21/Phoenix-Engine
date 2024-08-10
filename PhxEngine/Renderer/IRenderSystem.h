#pragma once

namespace phx::renderer
{
	class IRenderSystem
	{
	public:
		virtual ~IRenderSystem() = default;

		virtual void* CacheData() = 0;

		virtual void Render() = 0;

	};
}