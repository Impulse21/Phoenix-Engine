#include "AssetImporter_Obj.h"

#include <PhxRhi/PhxRhi.h>

#include <PhxCore/SystemTime.h>
#include <PhxEngine/JobSystem.h>

#include <PhxRenderer/MaterialAsset.h>
#include <PhxRenderer/TextureResource.h>

#include <PhxData/IVirtualFileSystem.h>
#include <PhxData/IStreamingManager.h>
#include <PhxData/AssetManager.h>

#include <PhxResource/ResourceSystem.h>

#include "MeshResourceCompiler.h"

#include <fast_obj/fast_obj.h>
//#include <meshoptimizer/meshoptimizer.h>

#include <PhxWorld/WorldMetadata.def.h>

using namespace phxed;
using namespace phx;
using namespace phx::data;
using namespace phx::renderer;

namespace
{
#if false
	void* memory_open(const char*, void* user_data)
	{
		auto* memory = static_cast<SpanMutable<uint8_t>*>(user_data);
		return memory->data();
	}
	void memory_close(void*, void*)
	{
		return;
	}
	size_t memory_read(void*, void* dst, size_t /*bytes*/, void* user_data)
	{
		auto* memory = static_cast<SpanMutable<uint8_t>*>(user_data);

		(void)(dst);
		dst = memory->begin();

		return memory->Size();
	}

	unsigned long memory_size(void* /*file*/, void* user_data)
	{
		auto* memory = static_cast<SpanMutable<uint8_t>*>(user_data);
		return memory->Size();
	}
#endif
}

phx::StringHash ObjImporter::GetAssetTypeHash() const
{
#if false
	return SceneBlueprint::StaticTypeHash();
#else
	return {};
#endif
}

void ObjImporter::ImportAsync(AssetManager* /*asset_manager*/, RefCountPtr<Asset> /*asset*/, std::string const& /*virtual_file_path*/) const
{
#if false
	RefCountPtr<SceneBlueprint> scene_blueprint = asset.As<SceneBlueprint>();

	auto vfs = IVirtualFileSystem::Ptr;
	Result<data::AsyncResourceDescriptor> resource_descriptor = vfs->GetResourceDescriptorForAsync(virtual_file_path);
	if (!resource_descriptor)
	{
		PHX_CORE_ERROR("[GLTF Handler] Failed to find file info '{0}'", virtual_file_path);
		scene_blueprint->state = Resource::State::Error;
		return;
	}

	scene_blueprint->state = Resource::State::Loading;

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

	// -- Main Load logic ---
	request.on_complete = [=](data::StreamingResult const& result) mutable {

		if (result.error_code != ErrorCode::Success)
		{
			scene_blueprint->state = Asset::State::Error;
			return;
		}

		fastObjCallbacks callbacks = {
			.file_open = memory_open,
			.file_close = memory_close,
			.file_read = memory_read,
			.file_size = memory_size
		};

		SpanMutable file_data(reinterpret_cast<uint8_t*>(dest.get()), resource_descriptor->length_of_resource);
		fastObjMesh* raw_obj = fast_obj_read_with_callbacks("", &callbacks, &file_data);
		if (!raw_obj)
		{
			PHX_CORE_ERROR("[OBJ Handler] Failed to parse obj file '{0}'", virtual_file_path);
			scene_blueprint->state = Resource::State::Error;
			return;
		}

		std::shared_ptr<fastObjMesh> obj_data_owner(raw_obj, &fast_obj_destroy);

		// Lets construct out material assets
		std::vector<scene::MaterialAssignment> mat_assignments(obj_data_owner->object_count);
		for (uint32_t i = 0; i < obj_data_owner->object_count; i++)
		{
			// Look up the material and process - no material merging yet
			for (uint32_t j = 0; j < obj_data_owner->material_count; j++)
			{
				
				fastObjGroup& group = obj_data_owner->objects[i];
				fastObjMaterial& mtl = obj_data_owner->materials[j];
				if (group.name != mtl.name)
					continue;


				mat_assignments[i] = scene::MaterialAssignment{
					.material_virutal_path = virtual_file_path + "#" + mtl.name,
					.geometry_index = i
				};

				auto mtl_asset = asset_manager->RegisterPrecreatedAsset<MaterialAsset>(mat_assignments[i].material_virutal_path.c_str());

				mtl_asset->parameters["ambient"] = hlslpp::float3(mtl.Ka[0], mtl.Ka[1], mtl.Ka[2]);
				mtl_asset->parameters["specular"] = hlslpp::float3(mtl.Ks[0], mtl.Ks[1], mtl.Ks[2]);
				mtl_asset->parameters["emissive"] = hlslpp::float3(mtl.Ke[0], mtl.Ke[1], mtl.Ke[2]);

				mtl_asset->texture_paths["base_colour"] = ProcessTexture(virtual_file_path, obj_data_owner->textures[mtl.map_Kd].path);
				mtl_asset->texture_paths["roughness"] = ProcessTexture(virtual_file_path, obj_data_owner->textures[mtl.map_Ns].path);
				mtl_asset->texture_paths["metalness"] = ProcessTexture(virtual_file_path, obj_data_owner->textures[mtl.map_Ni].path); // Not sure about this one
				mtl_asset->texture_paths["normal"] = ProcessTexture(virtual_file_path, obj_data_owner->textures[mtl.map_bump].path); // Not sure about this one

			}
		}

		auto rm = phx::ResourceSystem::Ptr;
		std::string mesh_virtual_path = virtual_file_path + "#Mesh_0";

		auto [mesh_placeholder, am_i_the_creator] = rm->FindOrCreatePlaceholder<MeshResource>(mesh_virtual_path);
		if (am_i_the_creator)
		{
			JobSystem::SubmitJob([mesh_placeholder, obj_data_owner](JobContext const&) {

				ProcessMesh(mesh_placeholder.Get(), obj_data_owner.get());

			}, JobSystem::Priority::Low);
		}

		// Create a node for the car body
		SceneNode node;
		node.name = "obj_asset";

		// Create and add the MeshComponent
		auto mesh_comp = std::make_unique<scene::MeshComponent>();
		mesh_comp->mesh_virtual_path = mesh_virtual_path; // The path to the mesh resource
		mesh_comp->mat_assignments = mat_assignments;

		node.components.push_back(std::move(mesh_comp));

		SceneNodeHandle handle = scene_blueprint->AddNode(std::move(node));
		scene_blueprint->root_node_indices.push_back(handle);

		scene_blueprint->state = Resource::State::Loaded;
	};

	IStreamingManager::Ptr->Submit(std::move(request));
#endif
}

Mesh phxed::ObjImporter::GenerateMeshIndices(Mesh const& /*mesh_src*/, std::vector<std::vector<uint32_t>>& /*geometry_remaps*/)
{
#if false
	// Mesh Optimizer

	const phxed::VertexStream& srcPositionStream = *mesh_src.GetVertexStream(phx::renderer::VertexStream_Position);
	const phxed::VertexStream& srcNormalStream = *mesh_src.GetVertexStream(phx::renderer::VertexStream_Normal);
	const phxed::VertexStream& srcUv0Stream = *mesh_src.GetVertexStream(phx::renderer::VertexStream_UV0);

	geometry_remaps.reserve(mesh_src.Geometry.size());

	phxed::Mesh processed_mesh = {};
	processed_mesh.Geometry.reserve(mesh_src.Geometry.size());

	size_t processed_mesh_total_vertices = 0;
	size_t processed_mesh_index_global_offset = 0;
	for (auto& geom : mesh_src.Geometry)
	{
		PHX_ASSERT(geom.is_indexed);
		const size_t geom_total_indices = geom.vertex_count;

		std::array<meshopt_Stream, 3> vertexStream =
		{
			meshopt_Stream{
				.data = srcPositionStream.Data.get() + geom.vertex_offset,
				.size = srcPositionStream.ElementStride,
				.stride = srcPositionStream.ElementStride,
			},
			meshopt_Stream{
				.data = srcNormalStream.Data.get() + geom.vertex_offset,
				.size = srcNormalStream.ElementStride,
				.stride = srcNormalStream.ElementStride,
			},
			meshopt_Stream{
				.data = srcUv0Stream.Data.get() + geom.vertex_offset,
				.size = srcUv0Stream.ElementStride,
				.stride = srcUv0Stream.ElementStride,
			},
		};

		auto& remap = geometry_remaps.emplace_back();
		remap.resize(geom_total_indices);

		size_t geom_total_vertices =
			meshopt_generateVertexRemapMulti(
				&remap[0],
				NULL,
				geom_total_indices,
				geom_total_indices,
				vertexStream.data(),
				vertexStream.size());

		processed_mesh.Indices.resize(processed_mesh.Indices.size() + geom_total_indices);
		meshopt_remapIndexBuffer(processed_mesh.Indices.data() + processed_mesh_index_global_offset, NULL, geom_total_indices, remap.data());

		processed_mesh_total_vertices += geom_total_indices;
		processed_mesh_index_global_offset += geom_total_indices;

		processed_mesh.Geometry.push_back(phxed::Mesh::GeometryData{
			.mat_assignment_id = geom.mat_assignment_id,
			.is_indexed = true,
			.vertex_count = static_cast<uint32_t>(geom_total_vertices),
			.index_offset = static_cast<uint32_t>(processed_mesh_index_global_offset),
			.index_count= static_cast<uint32_t>(geom_total_indices),
		});
	}

	phxed::VertexStream& pos_stream = processed_mesh.AddVertexStream<float>(phx::renderer::VertexStream_Position, 3, processed_mesh_total_vertices);
	phxed::VertexStream& normal_stream = processed_mesh.AddVertexStream<float>(phx::renderer::VertexStream_Normal, 3, processed_mesh_total_vertices);
	phxed::VertexStream& uv_stream = processed_mesh.AddVertexStream<float>(phx::renderer::VertexStream_UV0, 2, processed_mesh_total_vertices);
	size_t processed_mesh_vertex_global_offset = 0;

	for (size_t i = 0; i < processed_mesh.Geometry.size(); i++)
	{
		const size_t geom_total_indices = mesh_src.Geometry[i].vertex_count;

		meshopt_remapVertexBuffer(
			pos_stream.Data.get() + processed_mesh_vertex_global_offset,
			srcPositionStream.Data.get(),
			geom_total_indices,
			pos_stream.ElementStride,
			&geometry_remaps[i][0]);

		meshopt_remapVertexBuffer(
			normal_stream.Data.get() + processed_mesh_vertex_global_offset,
			srcNormalStream.Data.get(),
			geom_total_indices,
			normal_stream.ElementStride,
			&geometry_remaps[i][0]);

		meshopt_remapVertexBuffer(
			uv_stream.Data.get() + processed_mesh_vertex_global_offset,
			srcUv0Stream.Data.get(),
			geom_total_indices,
			uv_stream.ElementStride,
			&geometry_remaps[i][0]);
	}

#if false

	phxed::VertexStream* posStream = mesh.GetVertexStream(phx::renderer::VertexStream_Position);
	meshopt_Stream shadowStream =
		meshopt_Stream{
			.data = posStream->Data.get(),
			.size = posStream->ElementStride,
			.stride = posStream->ElementStride
	};

	// TODO: Shadow Buffer
	processed_mesh.ShadowIndices.resize(totalIndices);
	meshopt_generateShadowIndexBufferMulti(mesh.ShadowIndices.data(), mesh.ShadowIndices.data(), totalIndices, totalVertices, &shadowStream, 1);

	meshopt_optimizeVertexCache(mesh.ShadowIndices.data(), mesh.ShadowIndices.data(), totalIndices, totalVertices);
#endif
	return processed_mesh;
#else
return {};
#endif
}

void phxed::ObjImporter::OptimizeMesh(Mesh& /*mesh*/, std::vector<std::vector<uint32_t>>& /*geometry_remaps*/)
{
#if false
	PrintStatistics(mesh);
	for (size_t i = 0; i < mesh.Geometry.size(); i++)
	{
		auto& geom = mesh.Geometry[i];
		uint32_t* geom_indices = mesh.Indices.data() + geom.index_offset;

		// -- Optimize vertex cache ---
		meshopt_optimizeVertexCache(geom_indices, geom_indices, geom.index_count, geom.vertex_count);

		// -- Vertex optmized overdraw ---
		// Not in demo?

		// -- Vertex fetch optimization ---
		meshopt_optimizeVertexFetchRemap(geometry_remaps[i].data(), geom_indices, geom.index_count, geom.vertex_count);

		for (auto& vertexStreamOpt : mesh.VertexStreams)
		{
			if (!vertexStreamOpt.has_value())
				continue;

			phxed::VertexStream& stream = vertexStreamOpt.value();

			meshopt_remapVertexBuffer(stream.Data.get(), stream.Data.get(), geom.vertex_count, stream.ElementStride, geometry_remaps[i].data());
		}
	}
#endif
}

std::string phxed::ObjImporter::ProcessTexture(std::string const& base_viritual_path, const char* path)
{
	std::string virtual_path = base_viritual_path + "#" + path;
	auto rm = phx::ResourceSystem::Ptr;
	rm->FindOrCreatePlaceholder<renderer::TextureResource>(virtual_path);

	return virtual_path;
}

void phxed::ObjImporter::ProcessMesh(renderer::MeshResource* /*resource*/, fastObjMesh* /*obj*/)
{
#if false
	phxed::Mesh mesh;

	size_t totalIndices = 0;

	for (uint32_t i = 0; i < obj->face_count; ++i)
		totalIndices += 3 * (obj->face_vertices[i] - 2);

	phxed::VertexStream& positionStream = mesh.AddVertexStream<DirectX::XMFLOAT3>(phx::renderer::VertexStream_Position, totalIndices);
	phx::SpanMutable<DirectX::XMFLOAT3> positionData = positionStream.AsSpanMutable<DirectX::XMFLOAT3>();

	phxed::VertexStream& normalsStream = mesh.AddVertexStream<DirectX::XMFLOAT3>(phx::renderer::VertexStream_Normal, totalIndices);
	phx::SpanMutable<DirectX::XMFLOAT3> normalData = normalsStream.AsSpanMutable<DirectX::XMFLOAT3>();

	phxed::VertexStream& uv0Stream = mesh.AddVertexStream<DirectX::XMFLOAT2>(phx::renderer::VertexStream_UV0, totalIndices);
	phx::SpanMutable<DirectX::XMFLOAT2> uv0Data = uv0Stream.AsSpanMutable<DirectX::XMFLOAT2>();

	size_t global_vertex_offset = 0;
	for (uint32_t i = 0; i < obj->object_count; ++i)
	{
		const fastObjGroup& group = obj->objects[i];

		const size_t submesh_start_vertex = global_vertex_offset;

		size_t input_index_offset = group.index_offset;

		for (size_t iFace = 0; iFace < group.face_count; ++iFace)
		{
			uint32_t vertices_in_face = obj->face_vertices[group.face_offset + iFace];

			// We must have at least a triangle.
			if (vertices_in_face < 3)
			{
				input_index_offset += vertices_in_face; // Skip degenerate faces.
				continue;
			}

			// --- Robust Fan Triangulation ---
			// Cache the first vertex of the polygon for creating the fan.
			fastObjIndex first_v_idx = obj->indices[input_index_offset];

			// Process the rest of the vertices in the face to form a triangle fan.
			for (unsigned int iVert = 2; iVert < vertices_in_face; ++iVert)
			{
				// Get the indices for the other two vertices of the triangle.
				fastObjIndex prev_v_idx = obj->indices[input_index_offset + iVert - 1];
				fastObjIndex curr_v_idx = obj->indices[input_index_offset + iVert];

				// Define the 3 vertices of the current triangle in the fan.
				fastObjIndex triangle_indices[3] = { first_v_idx, prev_v_idx, curr_v_idx };

				// Write out the 3 vertices for this triangle.
				for (int k = 0; k < 3; ++k)
				{
					fastObjIndex gi = triangle_indices[k];

					positionData[global_vertex_offset] = {
						obj->positions[gi.p * 3 + 0],
						obj->positions[gi.p * 3 + 1],
						obj->positions[gi.p * 3 + 2],
					};
					normalData[global_vertex_offset] = {
						obj->normals[gi.n * 3 + 0],
						obj->normals[gi.n * 3 + 1],
						obj->normals[gi.n * 3 + 2],
					};
					uv0Data[global_vertex_offset] = {
						obj->texcoords[gi.t * 2 + 0],
						obj->texcoords[gi.t * 2 + 1],
					};
					global_vertex_offset++; // IMPORTANT: Increment master vertex cursor.
				}
			}

			// Correctly advance the input index offset by the number of vertices in the face.
			input_index_offset += vertices_in_face;
		} 
		
		mesh.Geometry.emplace_back(phxed::Mesh::GeometryData{
			.mat_assignment_id = i,
			.is_indexed = false,
			.vertex_count = static_cast<uint32_t>(global_vertex_offset - submesh_start_vertex),
			.vertex_offset = static_cast<uint32_t>(submesh_start_vertex),
		});
	}

	std::vector<std::vector<uint32_t>> geometry_remaps;
	mesh = GenerateMeshIndices(mesh, geometry_remaps);

	OptimizeMesh(mesh, geometry_remaps);

	phxed::CompiledResource compiled_resource;
	phxed::MeshResourceCompiler::Compile(mesh, compiled_resource);

	TypedView<MeshMetadata> mesh_metadata = compiled_resource.metadata_chunk.GetView<MeshMetadata>();

	resource->cpu_data_buffer = std::move(compiled_resource.chunks[0]);
	resource->gemoetry_buffer = RHI::CreateBuffer({
		.DebugName = "Geometry Buffer",
		.Size = mesh_metadata->GeometryBufferSize,
		.BindingFlags = RHI::BindingFlags::ShaderResource | RHI::BindingFlags::IndexBuffer,
		.MiscFlags = RHI::ResourceMiscFlags::BufferRaw,
		.InitialState = RHI::ResourceStates::Common,
		},
		compiled_resource.chunks[1].Data()
	);
#endif
}

void phxed::ObjImporter::PrintStatistics(Mesh const&)
{
#if false
	meshopt_VertexCacheStatistics vcs = meshopt_analyzeVertexCache(mesh.Indices.data(), mesh.Indices.size(), mesh.GetVertexCount(), kCacheSize, 0, 0);
	meshopt_VertexFetchStatistics vfs = meshopt_analyzeVertexFetch(mesh.Indices.data(), mesh.Indices.size(), mesh.GetVertexCount(), sizeof(Vertex));
	meshopt_OverdrawStatistics os = meshopt_analyzeOverdraw(mesh.Indices.data(), mesh.Indices.size(), &copy.vertices[0].px, mesh.GetVertexCount(), sizeof(Vertex));

	meshopt_VertexCacheStatistics vcs_nv = meshopt_analyzeVertexCache(mesh.Indices.data(), mesh.Indices.size(), mesh.GetVertexCount(), 32, 32, 32);
	meshopt_VertexCacheStatistics vcs_amd = meshopt_analyzeVertexCache(mesh.Indices.data(), mesh.Indices.size(), mesh.GetVertexCount(), 14, 64, 128);
	meshopt_VertexCacheStatistics vcs_intel = meshopt_analyzeVertexCache(mesh.Indices.data(), mesh.Indices.size(), mesh.GetVertexCount(), 128, 0, 0);

	printf("%-9s: ACMR %f ATVR %f (NV %f AMD %f Intel %f) Overfetch %f Overdraw %f in %.2f msec\n", name, vcs.acmr, vcs.atvr, vcs_nv.atvr, vcs_amd.atvr, vcs_intel.atvr, vfs.overfetch, os.overdraw, (end - start) * 1000);
#endif
}
