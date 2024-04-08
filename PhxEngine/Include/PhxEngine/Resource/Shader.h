#pragma once

#include <PhxEngine/Resource/Resource.h>
#include <PhxEngine/Resource/ResourceFileFormat.h>
#include <PhxEngine/RHI/PhxRHI.h>

namespace PhxEngine
{
	constexpr uint16_t CurrentVersion = 1;

	struct ShaderFileFormat
	{
		uint32_t ID;
		uint32_t Version;

		Region<void> ByteCode;
	};

	class Shader : public Resource
	{
	public:
		Shader() = default;
		Shader(RHI::ShaderHandle shader)
			: m_shaderHandle(shader)
		{
		}

		~Shader() = default;

		operator RHI::ShaderHandle& () { return this->m_shaderHandle;  }

	private:
		RHI::ShaderHandle m_shaderHandle;
	};
}

