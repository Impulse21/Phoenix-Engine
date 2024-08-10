#pragma once

namespace phx
{
	class IRenderSystem
	{
	public:
		virtual ~IRenderSystem() = default;

		virtual void* CacheData() = 0;

		virtual void Render() = 0;

	};
}