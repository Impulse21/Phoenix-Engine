#include "GltfImporters.h"

#include <PhxCore/Assert.h>
#include <PhxCore/StringHash.h>
#include <PhxCore/VFS.h>
#include <PhxCore/ThreadPool.h>
#include <PhxCore/Math.h>
#include <PhxData/DataTypeFactory.h>
#include <PhxWorld/Entity.h>
#include <PhxRenderer/MeshResourceHandler.h>
#include <cgltf.h>

#include <format>
using namespace phx;

namespace
{
	std::pair<const uint8_t*, size_t> CgltfBufferAccessor(const cgltf_accessor* accessor, size_t defaultStride)
	{
		// TODO: sparse accessor support
		const cgltf_buffer_view* view = accessor->buffer_view;
		const uint8_t* Data = (uint8_t*)view->buffer->data + view->offset + accessor->offset;
		const size_t stride = view->stride ? view->stride : defaultStride;
		return std::make_pair(Data, stride);
	}

	template<typename T>
	void SetBufferData(std::vector<T>& destBuffer, cgltf_attribute const& cgltfAttribute, uint32_t vertexOffset)
	{
		const int stride = cgltfAttribute.data->stride;
		const size_t vertexCount = cgltfAttribute.data->count;

		PHX_ASSERT(!cgltfAttribute.data->is_sparse);
		PHX_ASSERT(stride == sizeof(T));

		auto [vertexSrc, vertexStride] = CgltfBufferAccessor(cgltfAttribute.data, 0);
		destBuffer.resize(static_cast<size_t>(vertexOffset) + vertexCount);
		std::memcpy(
			destBuffer.data() + vertexOffset,
			vertexSrc,
			stride * vertexCount);
	}

	void ProcessMesh(MeshData& meshData, cgltf_mesh const& cgltfMesh)
	{
		meshData.Geometry.resize(cgltfMesh.primitives_count);
		meshData.Name = cgltfMesh.name;
		for (cgltf_size iPrim = 0; iPrim < cgltfMesh.primitives_count; iPrim++)
		{
			const cgltf_primitive& cgltfPrim = cgltfMesh.primitives[iPrim];
			MeshData::GeometryData& geoData = meshData.Geometry[iPrim];

			geoData.MaterialId = cgltfPrim.material->name;

			const uint32_t vertexOffset = static_cast<uint32_t>(meshData.Vertex_Positions.size());

			const size_t indexRemap[] = { 0,2,1 };

			if (cgltfPrim.indices)
			{
				// Read in the index data
				const int stride = cgltfPrim.indices->stride;
				const size_t indexCount = cgltfPrim.indices->count;
				const size_t indexOffset = meshData.Indices.size();

				meshData.Indices.resize(indexOffset + indexCount);
				geoData.IndexOffset = indexOffset;
				geoData.IndexCount = indexCount;

				auto [indexSrc, indexStride] = CgltfBufferAccessor(cgltfPrim.indices, 0);

				if (stride == 1)
				{
					for (size_t i = 0; i < indexCount; i += 3)
					{
						meshData.Indices[indexOffset + i + 0] = vertexOffset + indexSrc[i + indexRemap[0]];
						meshData.Indices[indexOffset + i + 1] = vertexOffset + indexSrc[i + indexRemap[1]];
						meshData.Indices[indexOffset + i + 2] = vertexOffset + indexSrc[i + indexRemap[2]];
					}
				}
				else if (stride == 2)
				{
					for (size_t i = 0; i < indexCount; i += 3)
					{
						meshData.Indices[indexOffset + i + 0] = vertexOffset + reinterpret_cast<const uint16_t*>(indexSrc)[i + indexRemap[0]];
						meshData.Indices[indexOffset + i + 1] = vertexOffset + reinterpret_cast<const uint16_t*>(indexSrc)[i + indexRemap[1]];
						meshData.Indices[indexOffset + i + 2] = vertexOffset + reinterpret_cast<const uint16_t*>(indexSrc)[i + indexRemap[2]];
					}
				}
				else if (stride == 4)
				{
					for (size_t i = 0; i < indexCount; i += 3)
					{
						meshData.Indices[indexOffset + i + 0] = vertexOffset + reinterpret_cast<const uint32_t*>(indexSrc)[i + indexRemap[0]];
						meshData.Indices[indexOffset + i + 1] = vertexOffset + reinterpret_cast<const uint32_t*>(indexSrc)[i + indexRemap[1]];
						meshData.Indices[indexOffset + i + 2] = vertexOffset + reinterpret_cast<const uint32_t*>(indexSrc)[i + indexRemap[2]];
					}
				}
				else
				{
					PHX_ASSERT(false && "unsupported index stride!");
				}
			}

			for (size_t iAttr = 0; iAttr < cgltfPrim.attributes_count; iAttr++)
			{
				const cgltf_attribute& cgltfAttribute = cgltfPrim.attributes[iAttr];
				const size_t vertexCount = cgltfAttribute.data->count;

				// -- Auto gen indices if we have too
				if (geoData.IndexCount == 0)
				{
					const size_t indexOffset = meshData.Indices.size();
					meshData.Indices.resize(indexOffset + vertexCount);
					for (size_t vi = 0; vi < vertexCount; vi += 3)
					{
						meshData.Indices[indexOffset + vi + 0] = uint32_t(vertexOffset + vi + indexRemap[0]);
						meshData.Indices[indexOffset + vi + 1] = uint32_t(vertexOffset + vi + indexRemap[1]);
						meshData.Indices[indexOffset + vi + 2] = uint32_t(vertexOffset + vi + indexRemap[2]);
					}
					geoData.IndexOffset = (uint32_t)indexOffset;
					geoData.IndexCount = (uint32_t)vertexCount;
				}



				switch (cgltfAttribute.type)
				{
				case cgltf_attribute_type_position:
					SetBufferData(meshData.Vertex_Positions, cgltfAttribute, vertexOffset);
					break;

				case cgltf_attribute_type_normal:
					SetBufferData(meshData.Vertex_Normals, cgltfAttribute, vertexOffset);
					break;

				case cgltf_attribute_type_tangent:
					SetBufferData(meshData.Vertex_Tangents, cgltfAttribute, vertexOffset);
					break;


				case cgltf_attribute_type_texcoord:
					if (std::strcmp(cgltfAttribute.name, "TEXCOORD_0") == 0)
					{
						SetBufferData(meshData.Vertex_Uvset_0, cgltfAttribute, vertexOffset);
					}
					else if (std::strcmp(cgltfAttribute.name, "TEXCOORD_1") == 0)
					{
						SetBufferData(meshData.Vertex_Uvset_1, cgltfAttribute, vertexOffset);
					}
					break;

				case cgltf_attribute_type_color:
					break;
				}
			}

#if false
			if (meshData.Vertex_Normals.empty())
			{
				// TODO: Create Normals
			}
#else
			PHX_ASSERT(!meshData.Vertex_Normals.empty())
#endif

		}
	}
}

bool phx::GltfMeshImporter::ImportImpl()
{
	m_out.resize(m_gltfData->meshes_count);
	for (cgltf_size iMesh = 0; iMesh < m_gltfData->meshes_count; iMesh++)
	{
		MeshData* meshData = &m_out[iMesh];
		const cgltf_mesh* cgltfMesh = &m_gltfData->meshes[iMesh];
		cgltf_size index = iMesh;
#if true
		ThreadPool::SubmitTask([index, cgltfMesh, meshData](){

				ProcessMesh(*meshData, *cgltfMesh);
				if (meshData->Name.empty())
					meshData->Name = std::format("mesh_{}", index);

		});
#else
		ProcessMesh(*meshData, *cgltfMesh);
#endif
	}

	ThreadPool::Wait();
	return true;
}

bool phx::GltfWorldImporter::ImportImpl()
{
	// Load Node Data
	cgltf_scene* gltfScene = m_gltfData->scene;

	for (size_t i = 0; i < gltfScene->nodes_count; i++)
	{
		// Load Node Data
		LoadNodeRec(*gltfScene->nodes[i], nullptr);
	}

	return false;
}

void phx::GltfWorldImporter::LoadNodeRec(cgltf_node const& gltfNode, Entity* parent)
{

	if (gltfNode.mesh)
	{
#if false
		// Create a mesh instance
		static size_t meshId = 0;

		std::string nodeName = gltfNode.name ? gltfNode.name : "Scene Node " + std::to_string(meshId++);
		entity = m_out.CreateEntity(nodeName);

		uint32_t meshInstance = 0;
		for (auto& mesh : this->m_meshEntityMap[gltfNode.mesh])
		{
			std::string meshNodeName = nodeName + std::to_string(meshInstance++);
#if false
			PhxEngine::Scene::Entity subEntity = scene.CreateEntity(meshNodeName);

			auto& instanceComponent = subEntity.AddComponent<MeshInstanceComponent>();
			instanceComponent.Mesh = mesh;

			subEntity.AddComponent<AABBComponent>();

			childEntities.push_back(subEntity);
#endif
		}
#endif
	}

	else if (gltfNode.camera)
	{
#if false
		static size_t cameraId = 0;
		std::string cameraName = gltfNode.camera->name ? gltfNode.camera->name : "Camera " + std::to_string(cameraId++);

		entity = scene.CreateEntity(cameraName);
		entity.AddComponent<CameraComponent>();
#else
		// TODO: Attach a camera component
#endif
	}
	else if (gltfNode.light)
	{
#if false
		static size_t lightID = 0;
		std::string lightName = gltfNode.light->name ? gltfNode.light->name : "Light " + std::to_string(lightID++);

		entity = scene.CreateEntity(lightName);
		auto& lightComponent = entity.AddComponent<LightComponent>();
		switch (gltfNode.light->type)
		{
		case cgltf_light_type_directional:
			lightComponent.Type = LightComponent::kDirectionalLight;
			lightComponent.Intensity = gltfNode.light->intensity > 0 ? (float)gltfNode.light->intensity : 6.0f;
			break;

		case cgltf_light_type_point:
			lightComponent.Type = LightComponent::kOmniLight;
			lightComponent.Intensity = gltfNode.light->intensity > 0 ? (float)gltfNode.light->intensity : 6.0f;
			break;

		case cgltf_light_type_spot:
			lightComponent.Type = LightComponent::kSpotLight;
			lightComponent.Intensity = gltfNode.light->intensity > 0 ? (float)gltfNode.light->intensity : 6.0f;
			break;

		case cgltf_light_type_invalid:
		default:
			// Ignore
			assert(false);
		}

		std::memcpy(
			&lightComponent.Colour.x,
			&gltfNode.light->color[0],
			sizeof(float) * 3);

		lightComponent.Range = gltfNode.light->range > 0 ? (float)gltfNode.light->range : std::numeric_limits<float>().max();
		lightComponent.InnerConeAngle = (float)gltfNode.light->spot_inner_cone_angle;
		lightComponent.OuterConeAngle = (float)gltfNode.light->spot_outer_cone_angle;
#else
		// TODO: Attach a light component
#endif
	}

	static size_t emptyNode = 0;


	std::string nodeName = gltfNode.name ? gltfNode.name : "Scene Node " + std::to_string(emptyNode++);
	Entity entity = m_out.CreateEntity(nodeName);

	TransformComponent& transform = entity.GetComponent<TransformComponent>();
	if (gltfNode.has_scale)
	{
		std::memcpy(
			&transform.Scale.x,
			&gltfNode.scale[0],
			sizeof(float) * 3);
	}
	if (gltfNode.has_rotation)
	{
		std::memcpy(
			&transform.Rotation.x,
			&gltfNode.rotation[0],
			sizeof(float) * 4);
	}
	if (gltfNode.has_translation)
	{
		std::memcpy(
			&transform.Translation.x,
			&gltfNode.translation[0],
			sizeof(float) * 3);
	}

	if (gltfNode.has_matrix)
	{
		DirectX::XMFLOAT4X4 WorldMatrix = math::cIdentityMatrix;
		std::memcpy(
			&WorldMatrix._11,
			&gltfNode.matrix[0],
			sizeof(float) * 16);


		DirectX::XMVECTOR scalar, rotation, translation;
		DirectX::XMMatrixDecompose(&scalar, &rotation, &translation, DirectX::XMLoadFloat4x4(&WorldMatrix));
		DirectX::XMStoreFloat3(&transform.Scale, scalar);
		DirectX::XMStoreFloat4(&transform.Rotation, rotation);
		DirectX::XMStoreFloat3(&transform.Translation, translation);
	}

	if (gltfNode.mesh)
	{
		auto& meshComp = entity.AddComponent<MeshComponent>();
		meshComp.Mesh = std::format("{}.{}", gltfNode.mesh->name, ResourceExtension<renderer::MeshResourceHandler>::value);
	}

	// GLTF default light Direciton is forward - I want this to be downwards.
#if false
	if (gltfNode.light)
	{
		transform.RotateRollPitchYaw(XMFLOAT3(XM_PIDIV2, 0, 0));
	}

	transform.UpdateTransform();

	if (cReverseZ)
	{
		transform.LocalTranslation.z *= -1.0f;
		transform.LocalRotation.x *= -1.0f;
		transform.LocalRotation.y *= 1.0f;
		transform.SetDirty();
		transform.UpdateTransform();
	}

	for (auto& child : childEntities)
	{
		child.AttachToParent(entity, true);
	}

#endif

	if (parent)
	{
		entity.AttachToParent(*parent);
	}

	for (cgltf_size i = 0; i < gltfNode.children_count; i++)
	{
		if (gltfNode.children[i])
			this->LoadNodeRec(*gltfNode.children[i], &entity);
	}
}
