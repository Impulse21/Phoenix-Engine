#pragma once

namespace phx
{
	class World;

	namespace WorldSerializer
	{
		bool Save(const char* filename, World& world);
		void Load(const char* filename, World& world);
	}
}