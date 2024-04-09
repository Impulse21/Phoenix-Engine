#pragma once

#include <memory>
#include <string>

namespace PhxEngine
{
	class ResourceHandler
	{
	public:
		virtual ~ResourceHandler() = 0;
	};

	namespace ResourceLoader
	{
		template<typename T>
		void RegisterHandler()
		{
			RegisterHandler(std::make_unique<T>());
		}

		void RegisterHandler(std::unique_ptr<ResourceHandler>&& handler) {};
		void Exists(std::string const& exists) {};
		void Load(std::string const& exists) {};

	}
}

