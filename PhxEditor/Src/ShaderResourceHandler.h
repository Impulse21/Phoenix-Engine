#pragma once

#include <PhxEngine/Resource/ResourceLoader.h>

namespace PhxEditor
{
	class ShaderResourceHandler final : public PhxEngine::ResourceHandler
	{
		PHX_OBJECT(ShaderResourceHandler, PhxEngine::ResourceHandler);

	public:
		ShaderResourceHandler() = default;
		~ShaderResourceHandler() = default;

		inline PhxEngine::StringHash GetResourceExt() { return this->m_extId; }

	private:
		PhxEngine::StringHash m_extId = PhxEngine::StringHash(".hlsl");
	};
}

