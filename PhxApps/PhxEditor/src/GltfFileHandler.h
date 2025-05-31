#pragma once

#include <PhxResource/IResourceFileHandler.h>
#include <PhxCore/StringHash.h>

namespace phxed
{
	class GltfFileHandler final : public phx::IResourceFileHandler
	{
	public:

		virtual phx::RefCountPtr<phx::Resource> LoadFromPak() const;
		virtual phx::RefCountPtr<phx::Resource> LoadLoose(const char* filename) const;
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

