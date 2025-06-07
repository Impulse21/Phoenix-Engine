#pragma once

#include <PhxResource/IResourceFileHandler.h>
#include <PhxCore/StringHash.h>

namespace phxed
{
	class GltfFileHandler final : public phx::IResourceFileHandler
	{
	public:

		virtual phx::RefCountPtr<phx::Resource> LoadAsync(phx::data::IVirtualFileSystem* vfs, phx::data::IAsyncIOSystem* loader, const char* virtual_file_path) const;
	};
}

namespace phx
{
	template<>
	struct ResourceFileExtension<phxed::GltfFileHandler>
	{
		static constexpr const char* value = ".gltf";
	};

	template<>
	struct ResourceFileHandlerId<phxed::GltfFileHandler>
	{
		static constexpr phx::StringHash value = "gltf"_hash;
	};
}

