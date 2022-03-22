#pragma once

#include <unordered_map>
#include <string>
#include "Shaders/ImGuiVS_compiled.h"
#include "Shaders/ImGuiPS_compiled.h"

namespace PhxEngine::Renderer
{
	using ShaderEntry = std::tuple<const unsigned char*, size_t>;

	static const std::unordered_map<std::string, ShaderEntry> sShaderDump =
	{
		{"ImGui_VS", { gImGuiVS, sizeof(gImGuiVS)}},
		{"ImGui_PS", { gImGuiPS, sizeof(gImGuiPS)}},
	};
}