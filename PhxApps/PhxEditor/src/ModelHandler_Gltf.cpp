#include "ModelHandler_Gltf.h"

#include <PhxCore/IO/FileUtils.h>
#include <PhxCore/Math.h>

#include <PhxData/IVirtualFileSystem.h>
#include <PhxData/IStreamingManager.h>

#include <PhxWorld/WorldMetadata.def.h>

#include <PhxResource/ResourceFile.h>
#include <PhxResource/ResourceSystem.h>

#include <PhxRenderer/ModelResoure.h>

#include <PhxEngine/JobSystem.h>

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

using namespace phxed;
using namespace phx;
using namespace phx::data;
using namespace phx::renderer;


phx::StringHash phxed::GtlfModelHandler::GetResourceTypeHash() const
{
	return ModelResoure::StaticTypeHash();
}

phx::RefCountPtr<phx::Resource> phxed::GtlfModelHandler::CreatePlaceholder() const
{
	return phx::RefCountPtr<phx::Resource>(new ModelResoure());
}

void phxed::GtlfModelHandler::LoadAsync(data::IStreamingManager* streaming_manager, IVirtualFileSystem* vfs, phx::RefCountPtr<phx::Resource> resource, std::string const& virtual_file_path) const
{
	// TODO: Check if cached version is loaded already. If so, load from there.
	RefCountPtr<ModelResoure> model_resource = resource.As<ModelResoure>();
	Result<data::AsyncResourceDescriptor> resource_descriptor = vfs->GetResourceDescriptorForAsync(virtual_file_path);

	if (!resource_descriptor)
	{
		PHX_CORE_ERROR("[GLTF Handler] Failed to find file info '{0}'", virtual_file_path);
		model_resource->state = Resource::State::Error;
		return;
	}

	model_resource->state = Resource::State::Loading;

	// TODO: Fix boiler plate stuff.
	std::shared_ptr<char[]> dest = std::make_shared<char[]>(resource_descriptor->length_of_resource);
	data::StreamingRequest request = {
		.operations = {
			{
				.source = {
					.data = resource_descriptor.GetValue(),
					.size = resource_descriptor->length_of_resource,
				},
				.destination = {
					.target = dest,
					.size = resource_descriptor->length_of_resource,
				}
			}
		}
	};

	request.on_complete = [=](data::StreamingResult const& /*result*/) mutable {
		PHX_CORE_ERROR("Loaded File '{0}'", virtual_file_path);
		};

	streaming_manager->Submit(std::move(request));
}
