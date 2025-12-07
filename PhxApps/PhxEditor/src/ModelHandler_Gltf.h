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

	class IStreamingManager;
}

namespace phxed
{
	class GtlfModelHandler final : public phx::ResourceFileHandler
	{
	public:
		phx::StringHash GetResourceTypeHash() const override;
		phx::RefCountPtr<phx::Resource> CreatePlaceholder() const override;
		void LoadAsync(IStreamingManager* streaming_manager, RefCountPtr<Resource> asset, AsyncResourceDescriptor const& resource_descriptor) const override;
		
	};
}
namespace phx 
{
		template<> 
		struct ResourceFileExtension<phxed::GtlfModelHandler> 
		{ 
			static constexpr const char* value = ".gltf";
		};
		
		template<>
		struct ResourceFileHandlerId<phxed::GtlfModelHandler> 
		{ 
			static constexpr phx::StringHash value = ".gltf"_hash;
		};
}