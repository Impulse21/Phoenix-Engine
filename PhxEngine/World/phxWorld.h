#pragma once

#include "entt/entt.hpp"

namespace phx
{
	class World final
	{
	public:
		World() = default;
		~World() = default;


	private:
		entt::registry m_registry;
	};
}

