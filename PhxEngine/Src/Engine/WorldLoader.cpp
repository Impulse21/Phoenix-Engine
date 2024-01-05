#include <PhxEngine/Engine/World.h>
#include <PhxEngine/Core/Log.h>
#include <PhxEngine/Core/Memory.h>

#include <PhxEngine/Engine/Components.h>
#include <PhxEngine/Engine/Entity.h>

#include <filesystem>
#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

using namespace PhxEngine;

namespace
{
	constexpr bool cReverseZ = true;

	struct CgltfContext
	{
		Core::IFileSystem* FileSystem;
		std::vector<std::shared_ptr<Core::IBlob>> Blobs;
	};

	class GltfWorldLoader : public World::IWorldLoader
	{
	public:
		bool LoadWorld(std::string const& filename, Core::IFileSystem* fileSystem, World::World& outWorld) override
		{

			CgltfContext cgltfContext =
			{
				.FileSystem = fileSystem,
				.Blobs = {}
			};

			cgltf_options options = { };
			options.file.read = nullptr;
			options.file.release = nullptr;
			options.file.user_data = &cgltfContext;

			std::unique_ptr<Core::IBlob> blob = fileSystem->ReadFile(filename);
			if (!blob)
			{
				return false;
			}

			cgltf_data* gltfObjects = nullptr;
			cgltf_result res = cgltf_parse(&options, blob->Data(), blob->Size(), &gltfObjects);
			if (res != cgltf_result_success)
			{
				PHX_LOG_ERROR("Couldn't load glTF file %s", filename.c_str());
				return false;
			}


			std::string rootName = std::filesystem::path(filename).stem().generic_string();;
			World::Entity rootEntity = outWorld.CreateEntity(rootName);

			// Parse World and dispatch Loading of assets
			for (size_t i = 0; i < gltfObjects->scene->nodes_count; i++)
			{
				// Load Node Data
				this->LoadNodeRec(*gltfObjects->scene->nodes[i], rootEntity, outWorld);
			}



			return true;
		}

	private:
		void LoadNodeRec(
			const cgltf_node& gltfNode,
			PhxEngine::World::Entity parent,
			World::World& world)
		{
			static size_t NodeId = 0;
			World::Entity entity;

			std::string nodeName = gltfNode.name ? gltfNode.name : "Scene Node " + std::to_string(NodeId++);
			entity = world.CreateEntity(nodeName);

			if (gltfNode.mesh)
			{
				// Create an entity with mesh Renderer
				static size_t meshId = 0;

				auto meshRenderComponent = entity.AddComponent<World::MeshRendererComponent>();
				// meshRenderComponent.Mesh = this->m_assetRegistry->Load<Assets::Mesh>(gltfNode.mesh->name);
			}

			if (gltfNode.camera)
			{
				entity.AddComponent<World::CameraComponent>();
			}

			if (gltfNode.light)
			{
				auto& lightComponent = entity.AddComponent<World::LightComponent>();

				switch (gltfNode.light->type)
				{
				case cgltf_light_type_directional:
					lightComponent.Type = World::LightComponent::kDirectionalLight;
					lightComponent.Intensity = gltfNode.light->intensity > 0 ? (float)gltfNode.light->intensity : 6.0f;
					break;

				case cgltf_light_type_point:
					lightComponent.Type = World::LightComponent::kOmniLight;
					lightComponent.Intensity = gltfNode.light->intensity > 0 ? (float)gltfNode.light->intensity : 6.0f;
					break;

				case cgltf_light_type_spot:
					lightComponent.Type = World::LightComponent::kSpotLight;
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
			}

			// -- Set Transforms ---
			auto& transform = entity.GetComponent<World::TransformComponent>();
			if (gltfNode.has_scale)
			{
				std::memcpy(
					&transform.LocalScale.x,
					&gltfNode.scale[0],
					sizeof(float) * 3);
				transform.SetDirty(true);
			}
			if (gltfNode.has_rotation)
			{
				std::memcpy(
					&transform.LocalRotation.x,
					&gltfNode.rotation[0],
					sizeof(float) * 4);

				transform.SetDirty(true);
			}
			if (gltfNode.has_translation)
			{
				std::memcpy(
					&transform.LocalTranslation.x,
					&gltfNode.translation[0],
					sizeof(float) * 3);

				transform.SetDirty(true);
			}
			if (gltfNode.has_matrix)
			{
				std::memcpy(
					&transform.WorldMatrix._11,
					&gltfNode.matrix[0],
					sizeof(float) * 16);
				transform.SetDirty(true);

				transform.ApplyTransform();
			}

			// GLTF default light Direciton is forward - I want this to be downwards.
			if (gltfNode.light)
			{
				transform.RotateRollPitchYaw(DirectX::XMFLOAT3(DirectX::XM_PIDIV2, 0, 0));
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

			if (parent)
			{
				entity.AttachToParent(parent, true);
			}

			for (int i = 0; i < gltfNode.children_count; i++)
			{
				if (gltfNode.children[i])
					this->LoadNodeRec(*gltfNode.children[i], entity, world);
			}
		}

		void LoadMeshes()
		{

		}

	private:
	};
}
std::unique_ptr<World::IWorldLoader> PhxEngine::World::WorldLoaderFactory::Create()
{
	return std::make_unique<GltfWorldLoader>();
}
