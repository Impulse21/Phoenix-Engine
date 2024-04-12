#pragma once

#include <memory>
#include <string>
#include <PhxEngine/Core/StringHash.h>
#include <PhxEngine/Core/Object.h>

namespace PhxEngine
{
	class ResourceHandler : public Object
	{
		PHX_OBJECT(ResourceHandler, Object);

	public:
		virtual ~ResourceHandler() = default;

		virtual StringHash GetResourceExt() = 0;

	};

	namespace ResourceLoader
	{
		void RegisterHandler(std::unique_ptr<ResourceHandler>&& handler);
		template<typename T, typename... Args>
		void RegisterHandler(Args&&... args)
		{
			RegisterHandler(std::make_unique<T>(std::forward<Args>(args)...));
		}

		bool RegonizedPath(std::string_view path);
		void Exists(std::string const& exists);
		void Load(std::string const& exists);

	}
}

