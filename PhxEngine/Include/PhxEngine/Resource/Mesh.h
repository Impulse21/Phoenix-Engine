#pragma once

#include <PhxEngine/Resource/Resource.h>
#include <PhxEngine/Resource/ResourceManager.h>


namespace PhxEngine
{
	class Mesh : public Resource
	{
		PHX_OBJECT(Mesh, Resource);
	public:
		
	private:

	};

	class MeshResourceRegistry : public ResourceRegistry<Mesh>
	{
	public:
		inline static MeshResourceRegistry* Ptr = nullptr;
	public:
		
		MeshResourceRegistry() = default;

		RefCountPtr<Mesh> Retrieve(StringHash meshId) override;
	};

	class MeshResourceFileHandler final : public IResourceFileHanlder
	{
	public:
		MeshResourceFileHandler() = default;

		std::string_view GetFileExtension() const override { return ".pmesh"; }

		void RegisterResourceFile(std::string_view file);

	private:
		struct File
		{
			std::string Filename;
		};

		std::vector<File> m_files;
	};
}

