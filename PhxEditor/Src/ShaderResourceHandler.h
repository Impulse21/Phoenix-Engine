#pragma once

#include <PhxEngine/Resource/ResourceStore.h>
#include <PhxEngine/Resource/Shader.h>

namespace PhxEditor
{
	class ShaderResourceHandler final : public PhxEngine::ResourceHandler
	{
		PHX_OBJECT(ShaderResourceHandler, PhxEngine::ResourceHandler);

	public:
		ShaderResourceHandler() = default;
		~ShaderResourceHandler() = default;

		inline PhxEngine::StringHash GetResourceExt() override { return this->m_extId; }
		inline PhxEngine::StringHash GetResourceType() override { return PhxEngine::Shader::GetTypeStatic(); }

		PhxEngine::RefCountPtr<PhxEngine::Resource> Load(std::unique_ptr<PhxEngine::IBlob>&& fileData) override;

		void LoadDispatch(std::string const& path) override;
		

	private:
		PhxEngine::StringHash m_extId = PhxEngine::StringHash(".hlsl");
	};
}

