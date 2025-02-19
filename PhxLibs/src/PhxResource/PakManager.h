#pragma once

#include "PhxCore/IO/PakFile.h"

namespace phx
{
	class PakManager
	{
	public:
		static void Mount(std::string const& filename);

		static void DrawGui();
	private:
		inline static std::vector<std::unique_ptr<PakFile>> ms_mountedPaks;
	};
}

