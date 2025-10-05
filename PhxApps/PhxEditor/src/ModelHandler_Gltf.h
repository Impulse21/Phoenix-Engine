#pragma once

#include <PhxResource/IResourceFileHandler.h>
#include <PhxData/IStreamingManager.h>
#include <PhxData/IVirtualFileSystem.h>

#include <PhxCore/StringHash.h>

// -- forward declares ---
struct cgltf_data;
struct cgltf_mesh;
struct cgltf_node;

namespace phx
{
	namespace JobSystem
	{
		struct Barrier;
	}

}

namespace phxed
{
	class GtlfModelHandler final : public phx::ResourceFileHandler
	{
	public:
		phx::StringHash GetResourceTypeHash() const override;
		phx::RefCountPtr<phx::Resource> CreatePlaceholder() const override;
		void LoadAsync(phx::ResourceSystem* resource_system, phx::RefCountPtr<phx::Resource> asset, std::string const& virtual_file_path) const override;

	};
}

PHX_DEFINE_RES_FILE_EXT(phxed::GtlfModelHandler, ".gltf")