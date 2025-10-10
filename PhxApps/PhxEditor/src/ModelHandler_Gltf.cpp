#include "ModelHandler_Gltf.h"

#include <PhxCore/IO/FileUtils.h>
#include <PhxCore/Math.h>

#include <PhxData/IVirtualFileSystem.h>
#include <PhxData/IStreamingManager.h>

#include <PhxWorld/WorldMetadata.def.h>

#include <PhxResource/ResourceFile.h>
#include <PhxResource/ResourceSystem.h>

#include <PhxRenderer/ModelResoure.h>
#include <PhxRenderer/ModelImporter_Gltf.h>

#include <PhxEngine/JobSystem.h>

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

using namespace phxed;
using namespace phx;
using namespace phx::data;
using namespace phx::renderer;

namespace
{
	void OnFileLoaded(RefCountPtr<ModelResoure>& model_resource, SpanMutable<uint8_t> file_data)
	{
		ImportOptions options = {};
		Result<phx::renderer::ModelData> model_data = ModelImporter_Gltf::Import(options, file_data);

		if (model_data.HasError())
		{
			model_resource->state = Resource::State::Error;
			return;
		}

		// TODO Compile resource save and cache. Dispatch loading dependencies.
	}
}

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

	request.on_complete = [=](data::StreamingResult const& result) mutable {
		if (result.error_code == ErrorCode::Success)
		{
			SpanMutable file_data(reinterpret_cast<uint8_t*>(dest.get()), resource_descriptor->length_of_resource);
			OnFileLoaded(model_resource, file_data);
		}
		else
		{
			PHX_CORE_ERROR("Failed to load '{0}'", virtual_file_path);
			model_resource->state = Resource::State::Error;
		}
	};

	streaming_manager->Submit(std::move(request));
}
