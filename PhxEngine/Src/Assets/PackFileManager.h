#pragma once

#include <string>
#include <memory>
#include <PhxEngine/Assets/PhxArchFile.h>

namespace PhxEngine::Assets
{
	class PackFileManager
	{
	public:
		static void Initialize();
		static void Finalize();

		static void AddPktFile(std::string const& fileanme);

	private:
		struct File
		{
			std::string Filename;
			std::unique_ptr<PhxPktFile> PktFile;
		};

	};
}

