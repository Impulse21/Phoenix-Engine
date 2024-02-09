#pragma once

#include <memory>
#include <string>
#include <PhxEngine/Core/FlexArray.h>
#include <PhxEngine/Assets/PhxArchFile.h>

namespace PhxEngine::Assets
{
	class IPktFileFactory;
	using FileId = size_t;

	class PhxPktFileManager
	{
	public:

		static void Initialize(std::unique_ptr<IPktFileFactory>&& factory);
		static void Finalize();
		static void Update();

		static void RenderDebugWindow();

		static FileId AddFile(std::string const& filename);

	private:
		struct File
		{
			std::string Filename;
			std::unique_ptr<PhxPktFile> PktFile;
		};
		inline static PhxEngine::Core::FlexArray<File> m_files;
		inline static std::unique_ptr<IPktFileFactory> m_pktFileFactory;
	};
}

