#include "AssetImporter_Obj.h"

#include <PhxRhi/GfxDevice.h>

#include <PhxCore/SystemTime.h>
#include <PhxEngine/JobSystem.h>

#include <PhxRenderer/MeshResource.h>
#include <PhxRenderer/MaterialAsset.h>
#include <PhxRenderer/TextureResource.h>

#include <PhxData/IVirtualFileSystem.h>
#include <PhxData/IAsyncIOSystem.h>
#include <PhxData/AssetManager.h>

#include <PhxResource/ResourceSystem.h>

#include "MeshResourceCompiler.h"

#include <fast_obj/fast_obj.h>
#include <meshoptimizer/meshoptimizer.h>

#include <PhxWorld/WorldMetadata.def.h>

using namespace phxed;
using namespace phx;
using namespace phx::data;
using namespace phx::renderer;

namespace
{
	void* memory_open(const char*, void*)
	{
		return nullptr;
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
}

phx::StringHash ObjImporter::GetAssetTypeHash() const
{
	return SceneBlueprint::StaticTypeHash();
}

void ObjImporter::ImportAsync(AssetManager* asset_manager, RefCountPtr<Asset> asset, std::string const& virtual_file_path) const
{
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

	data::AsyncReadRequest request = {
		.resource_descriptor = resource_descriptor.GetValue(),
		.bytes_to_read = resource_descriptor->length_of_resource,
	};

	// -- Main Load logic ---
	request.callback = [=](data::AsyncReadResult const& result) mutable {

		fastObjCallbacks callbacks = {
			.file_open = memory_open,
			.file_close = memory_close,
			.file_read = memory_read,
			.file_size = memory_size
		};

		SpanMutable file_data(const_cast<uint8_t*>(result.data_buffer.data()), result.bytes_actually_read);
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

				ProcessMesh(mesh_placeholder, obj_data_owner.get());

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

		scene_blueprint->nodes.push_back(std::move(node));

		scene_blueprint->state = Resource::State::Loaded;
	};

	IAsyncIOSystem::Ptr->QueueRead(std::move(request));
}


Mesh phxed::ObjImporter::GenerateMeshIndices(Mesh const& meshSrc, std::vector<uint32_t>& outRemap)
{
	// Mesh Optimizer

	phxed::Mesh processedMesh = {};
	const size_t totalIndices = meshSrc.GetVertexCount();

	const phxed::VertexStream& srcPositionStream = *meshSrc.GetVertexStream(phx::renderer::VertexStream_Position);
	const phxed::VertexStream& srcNormalStream = *meshSrc.GetVertexStream(phx::renderer::VertexStream_Normal);
	const phxed::VertexStream& srcUv0Stream = *meshSrc.GetVertexStream(phx::renderer::VertexStream_UV0);

	std::array<meshopt_Stream, 3> vertexStream =
	{
		meshopt_Stream{
			.data = srcPositionStream.Data.get(),
			.size = srcPositionStream.ElementStride,
			.stride = srcPositionStream.ElementStride,
		},
		meshopt_Stream{
			.data = srcNormalStream.Data.get(),
			.size = srcNormalStream.ElementStride,
			.stride = srcNormalStream.ElementStride,
		},
		meshopt_Stream{
			.data = srcUv0Stream.Data.get(),
			.size = srcUv0Stream.ElementStride,
			.stride = srcUv0Stream.ElementStride,
		},
	};

	outRemap.clear();
	outRemap.resize(totalIndices);
	std::vector<uint32_t>& remap = outRemap;
	size_t totalVertices =
		meshopt_generateVertexRemapMulti(
			&remap[0],
			NULL,
			totalIndices,
			totalIndices,
			vertexStream.data(),
			vertexStream.size());


	processedMesh.Indices.resize(totalIndices);
	meshopt_remapIndexBuffer(processedMesh.Indices.data(), NULL, totalIndices, remap.data());

	phxed::VertexStream& posStream = processedMesh.AddVertexStream<DirectX::XMFLOAT3>(phx::renderer::VertexStream_Position, totalVertices);
	meshopt_remapVertexBuffer(
		posStream.Data.get(),
		srcPositionStream.Data.get(),
		totalIndices,
		posStream.ElementStride,
		&remap[0]);

	phxed::VertexStream& normalStream = processedMesh.AddVertexStream<DirectX::XMFLOAT3>(phx::renderer::VertexStream_Normal, totalVertices);
	meshopt_remapVertexBuffer(
		normalStream.Data.get(),
		srcNormalStream.Data.get(),
		totalIndices,
		normalStream.ElementStride,
		&remap[0]);

	phxed::VertexStream& uvStream = processedMesh.AddVertexStream<DirectX::XMFLOAT2>(phx::renderer::VertexStream_UV0, totalVertices);
	meshopt_remapVertexBuffer(
		uvStream.Data.get(),
		srcUv0Stream.Data.get(),
		totalIndices,
		uvStream.ElementStride,
		&remap[0]);


	PHX_WARN("Hard coding a single mateiral entry into the geometry. Please fix this");
	PHX_ASSERT(meshSrc.Geometry.size() == 1);
	processedMesh.Geometry.push_back(meshSrc.Geometry[0]);
	processedMesh.Geometry[0].IndexCount = totalIndices;

	return processedMesh;
}

void phxed::ObjImporter::OptimizeMesh(Mesh& mesh, std::vector<uint32_t>& remap)
{
	const size_t totalIndices = mesh.Indices.size();
	const size_t totalVertices = mesh.GetVertexCount();

	PrintStatistics(mesh);
	phx::CpuTimer timer;
	// -- Optimize vertex cache ---
	meshopt_optimizeVertexCache(mesh.Indices.data(), mesh.Indices.data(), totalIndices, totalVertices);

	// -- Vertex optmized overdraw ---
	// Not in demo?

	// -- Vertex fetch optimization ---
	meshopt_optimizeVertexFetchRemap(remap.data(), mesh.Indices.data(), totalIndices, totalVertices);

	for (auto& vertexStreamOpt : mesh.VertexStreams)
	{
		if (!vertexStreamOpt.has_value())
			continue;

		phxed::VertexStream& stream = vertexStreamOpt.value();

		meshopt_remapVertexBuffer(stream.Data.get(), stream.Data.get(), totalVertices, stream.ElementStride, remap.data());
	}

	phx::CpuTimeStep optimizeTime = timer.Elapsed();

	timer.Reset();
	phxed::VertexStream* posStream = mesh.GetVertexStream(phx::renderer::VertexStream_Position);
	meshopt_Stream shadowStream =
		meshopt_Stream{
			.data = posStream->Data.get(),
			.size = posStream->ElementStride,
			.stride = posStream->ElementStride
	};

	mesh.ShadowIndices.resize(totalIndices);
	meshopt_generateShadowIndexBufferMulti(mesh.ShadowIndices.data(), mesh.ShadowIndices.data(), totalIndices, totalVertices, &shadowStream, 1);

	meshopt_optimizeVertexCache(mesh.ShadowIndices.data(), mesh.ShadowIndices.data(), totalIndices, totalVertices);
	phx::CpuTimeStep shadowOptimize = timer.Elapsed();

	PHX_INFO(
		"Deintrlvd: {0} vertices, optimized in {1} msec, generated & optimized shadow indices in {2} msec",
		totalVertices,
		optimizeTime.GetMilliseconds(),
		shadowOptimize.GetMilliseconds());

	PrintStatistics(mesh);
}

std::string phxed::ObjImporter::ProcessTexture(std::string const& base_viritual_path, const char* path)
{
	std::string virtual_path = base_viritual_path + "#" + path;
	auto rm = phx::ResourceSystem::Ptr;
	rm->FindOrCreatePlaceholder<renderer::TextureResource>(virtual_path);

	return virtual_path;
}

void phxed::ObjImporter::ProcessMesh(renderer::MeshResource* resource, fastObjMesh* obj)
{
	(void)resource;
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
			.IndexOffset = static_cast<uint32_t>(submesh_start_vertex),
			.IndexCount = static_cast<uint32_t>(global_vertex_offset- submesh_start_vertex),
		});
	}

	std::vector<uint32_t> remap;
	mesh = GenerateMeshIndices(mesh, remap);

	OptimizeMesh(mesh, remap);
	phxed::CompiledResource compiled_resource;
	phxed::MeshResourceCompiler::Compile(mesh, compiled_resource);
	(void)compiled_resource;
	// TODO:
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
