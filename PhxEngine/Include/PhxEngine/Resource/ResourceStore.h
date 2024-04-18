#pragma once

#include <memory>
#include <string>
#include <filesystem>
#include <PhxEngine/Core/StringHash.h>
#include <PhxEngine/Core/Object.h>
#include <PhxEngine/Resource/Resource.h>

namespace PhxEngine
{
	class IBlob;
	class ResourceHandler : public Object
	{
		PHX_OBJECT(ResourceHandler, Object);

	public:
		virtual ~ResourceHandler() = default;

		virtual StringHash GetResourceExt() = 0;
		virtual StringHash GetResourceType() = 0;

		virtual RefCountPtr<Resource> Load(std::unique_ptr<IBlob>&& fileData) = 0;

		virtual void LoadDispatch(std::string const& path) = 0;
	};


	namespace ResourceStore
	{
		void RegisterHandler(std::unique_ptr<ResourceHandler>&& handler);
		template<typename T, typename... Args>
		void RegisterHandler(Args&&... args)
		{
			RegisterHandler(std::make_unique<T>(std::forward<Args>(args)...));
		}

		void MountResourceDir(std::string const& path);
		void MountResourceDir(std::filesystem::path const& path);
		// void MountResourcePack(std::string const& path);

		RefCountPtr<Resource> GetResource(StringHash type, std::string const& name);

		template<typename T>
		RefCountPtr<T> GetResource(std::string const& name)
		{
			static_assert(std::is_base_of_v<Object, T>, "T must extend Object");
			return GetResource(T::GetTypeStatic(), name).As<T>();
		}

		void RunGarbageCollection();
	}
}

