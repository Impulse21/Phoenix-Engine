#pragma once

#include <PhxEngine/Core/VirtualFileSystem.h>
#include <string>

namespace PhxEngine::RHI::ShaderCompiler
{
	void Compile(std::string const& filename, PhxEngine::Core::IFileSystem* fileSystem);
}

