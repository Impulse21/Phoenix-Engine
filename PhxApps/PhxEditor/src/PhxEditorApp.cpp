
#include <PhxCore/Base.h>
#include <PhxCore/SystemTime.h>
#include <PhxCore/Profiler.h>

#include <PhxRhi/PhxRhi.h>

#include <PhxWorld/WorldComponents.h>
#include <PhxWorld/Entity.h>
#include <PhxWorld/World.h>
#include <PhxWorld/WorldSerializer.h>
#include <PhxWorld/WorldMetadata.def.h>

#include <PhxData/IVirtualFileSystem.h>
#include <PhxData/IStreamingManager.h>
#include <PhxData/AssetManager.h>

#include <PhxResource/ResourceSystem.h>

#include <PhxRenderer/RenderSystem.h>
#include <PhxRenderer/RenderLayers/MeshRenderLayer.h>

#include <PhxCore/IO/FileUtils.h>

#include <PhxEngine/EntryPoint.h>

#include <Generated/GlobalVariables.h>

#include <random>

#include "AssetImporter_Gltf.h"
#include "AssetImporter_Obj.h"


// -- Renderer Includes ---
#include <PhxEngine/Memory/FrameMemoryManager.h>
#include <PhxRenderer/MeshResource.h>

// static const char* kDefault3DModel = "art://Sponza/glTF/Sponza.gltf";
static const char* kDefault3DModel = "art://SM_Chest_01.obj"; 

#define InjectDefault3DModel() \
    if (raptor::file_exists(kDefault3DModel)) {\
        argc = 2;\
        argv[1] = const_cast<char*>(kDefault3DModel);\
    }\
    else {\
       printf("Unable to find default model. Please check the README in the root folder and make sure you've run `python ./bootstrap.py` to download all the additional assets for this project.\n");\
       exit(-1);\
    }

struct View
{
	hlslpp::float4x4 view_matrix;
	hlslpp::float4x4 projection_matrix;
	hlslpp::float4x4 world_to_clip_matrix; // View - project matrix

	hlslpp::float4x4 inv_view_matrix;
	hlslpp::float4x4 inv_projection_matrix;
	hlslpp::float4x4 inv_world_to_clip_matrix;
};

struct Drawable
{
	phx::RefCountPtr<phx::renderer::MeshResource> mesh_resource;
};

struct ForwardPassDrawData
{
	size_t		num_drawables;
	Drawable*	drawables;
};

struct FrameRenderData
{
	uint32_t num_views = 0;
	View* views = nullptr;

	// [ view, render_pass, List of cachedData
	void** cached_data = nullptr;
};

class PhxEditor final : public phx::IApplication
{
public:
	static PhxEditor* Instance() { return ms_instance; }

	PhxEditor(const phx::ApplicationDescriptor& desc)
		: m_desc(desc)
	{
		ms_instance = this;
	}

	~PhxEditor() { ms_instance = nullptr; }

	void Startup() override;
	void Shutdown() override;

	void OnPreRender() override;
	void OnUpdate_Threaded(float deltaTime) override;
	void OnRender_Threaded() override;

	const char* GetName() const override { return this->m_desc.Name.c_str(); }
	void GetDefaultWindowSize(uint32_t& outWidth, uint32_t& outHeight) const override
	{
		outWidth = m_desc.Width;
		outHeight = m_desc.Height;
	}

	void SetWindowHandle(void* handle) override { m_window_handle = handle; }
	void* GetWindowHandle() const override { return m_window_handle; }

private:
	void TEST_RotateEntity(float deltaTime, phx::TransformComponent& comp);

private:
	inline static PhxEditor* ms_instance = nullptr;
	const phx::ApplicationDescriptor m_desc;
	void* m_window_handle;
	
	FrameRenderData m_per_frame_cache;
	phx::World m_world;

	phx::RefCountPtr<phx::SceneBlueprint> m_scene_blueprint;
};

phx::IApplication* phx::CreateApplication()
{
	ApplicationDescriptor desc = {
		.Name = "PhxEditor",
		.WorkingDirectory = phx::GetDirectoryWithExecutable()
	};

	return new PhxEditor(desc);
}

void phx::DeleteApplication(phx::IApplication* ptr)
{
	delete ptr;
}

void PhxEditor::Startup()
{
	{
		auto vfs = phx::data::IVirtualFileSystem::Ptr;
		vfs->Mount("res://", phx::GlobalPaths::DefaultProjectDir);
		vfs->Mount("art://", phx::GlobalPaths::ArtSrcDirectory);
		vfs->Mount("res_embedded://", "embedded://");
		// TODO: TRY mounting a pack
	}

	auto* asset_manager = phx::data::AssetManager::Ptr;
	asset_manager->RegisterImporter<phxed::GltfFileImporter>();
	asset_manager->RegisterImporter<phxed::ObjImporter>();

	m_scene_blueprint = asset_manager->Get<phx::SceneBlueprint>(kDefault3DModel);

	phx::Entity camera_entity = m_world.CreateEntity("Debug_Camera");
	auto& debug_camera_comp = camera_entity.AddComponent<phx::CameraComponent>();
	
	uint32_t width, height;
	GetDefaultWindowSize(width, height);
	debug_camera_comp.width = width;
	debug_camera_comp.height = height;
	debug_camera_comp.eye = { 0.0f, 5.0f, 10.0f };
	debug_camera_comp.active = true;


#if false
	phx::gfx::IRenderSystem::Ptr->AddLayer<phx::gfx::MeshRenderLayer>();


	// TODO: Incremental build this to save time.
	// however, still testing, so generating this every time at startup is fine.

	const char* defaultWorldFilename = "res://Default.phxwld";

	PHX_INFO("Compiling Test Resources");
	GenerateTestResources(defaultWorldFilename);

	// Assume there is a default world that can be loaded

	PHX_INFO("Loading Test Resources");
	
	// Register observers
	phx::gfx::IRenderSystem::Ptr->RegisterObservers(m_world);

	if (!phx::WorldSerializer::Load(fs, defaultWorldFilename, m_world))
		PHX_ERROR("Failed to load world '{0}'", defaultWorldFilename);
#endif
}

void PhxEditor::Shutdown()
{
}

void PhxEditor::OnPreRender()
{
	PHX_PROFILE;
	{
		PHX_PROFILE_SECTION("Construct View");

		auto cameras_view = m_world.GetAllEntitiesWith<phx::NameComponent, phx::CameraComponent>();
		for (auto e : cameras_view)
		{
			auto& [name_comp, camera_comp] = cameras_view.get<phx::NameComponent, phx::CameraComponent>(e);

			if (camera_comp.active)
			{
				m_per_frame_cache.num_views++;
			}
		}

		m_per_frame_cache.views = phx_new_frame View[m_per_frame_cache.num_views];

		size_t i_view = 0;
		for (auto e : cameras_view)
		{
			auto& [name_comp, camera_comp] = cameras_view.get<phx::NameComponent, phx::CameraComponent>(e);

			if (!camera_comp.active)
				continue;

			const float near_z = camera_comp.z_near;
			const float far_z = camera_comp.z_far;

			auto view_matrix = hlslpp::float4x4::look_at(
				camera_comp.eye,
				camera_comp.forward,
				camera_comp.up);

			auto& view = m_per_frame_cache.views[i_view++];

			view.view_matrix = view_matrix;
			view.inv_view_matrix = hlslpp::inverse(view_matrix);

			float aspect_ratio = camera_comp.width / camera_comp.height;
			hlslpp::projection projection(
				hlslpp::frustum::field_of_view_x(camera_comp.fov, aspect_ratio, near_z, far_z),
				hlslpp::zclip::zero);

			view.projection_matrix = hlslpp::float4x4::perspective(projection);
			view.inv_projection_matrix = hlslpp::inverse(view.projection_matrix);

			// -- VP
			view.world_to_clip_matrix = view_matrix * view.projection_matrix;
			view.inv_world_to_clip_matrix = hlslpp::inverse(view.world_to_clip_matrix);
		}

		{
			PHX_PROFILE_SECTION("Construct draw");
			auto* draw_data = phx_new_frame ForwardPassDrawData;
			auto drawable_view = m_world.GetAllEntitiesWith<phx::MeshComponent, phx::TransformComponent>();
			for (auto e : drawable_view)
			{
				auto& [mesh_comp, transform_comp] = drawable_view.get<phx::MeshComponent, phx::TransformComponent>(e);
				if (!mesh_comp.Mesh->IsLoaded())
					continue;

				draw_data->num_drawables++;
			}

			draw_data->drawables = phx_new_frame Drawable[draw_data->num_drawables];

			size_t i_drawable = 0;
			for (auto e : drawable_view)
			{
				auto& [mesh_comp, transform_comp] = drawable_view.get<phx::MeshComponent, phx::TransformComponent>(e);
				if (!mesh_comp.Mesh->IsLoaded())
					continue;

				Drawable& drawable = draw_data->drawables[i_drawable++];
				drawable.mesh_resource = mesh_comp.Mesh.As<phx::renderer::MeshResource>();
			}

			m_per_frame_cache.cached_data = phx_new_frame void*[1];
			*m_per_frame_cache.cached_data = draw_data;
		}
	}
}

void PhxEditor::OnUpdate_Threaded(float delta_time)
{
	PHX_PROFILE;

	phx::data::IStreamingManager::Ptr->Tick(delta_time);
	if (m_scene_blueprint->state == phx::data::Asset::State::Loaded)
	{
		m_world.InstantiateFrom(*m_scene_blueprint);
	}
	// Rotate cube in a random direction
}

void PhxEditor::OnRender_Threaded()
{
	PHX_PROFILE;

	phx::RHI::CommandBufferHandle cmd_buffer = phx::RHI::BeginFrameCommandBuffer();
	ForwardPassDrawData* pass_data = static_cast<ForwardPassDrawData*>(m_per_frame_cache.cached_data[0]);
	for (size_t i = 0; i < m_per_frame_cache.num_views; i++)
	{
		View& view = m_per_frame_cache.views[i];

		for (size_t i_drawable = 0; i_drawable < pass_data->num_drawables; i_drawable++)
		{
			Drawable& drawable = pass_data->drawables[i_drawable];

			// Bind Buffer data
			for (auto& draw_info : drawable.mesh_resource->cpu_data->Draw)
			{
				phx::RHI::CommandRecorder::DrawIndexed(
					cmd_buffer,
					draw_info.IndexCount,
					draw_info.StartIndex,
					draw_info.BaseVertex);
			}
		}
	}

	phx::RHI::SubmitAndPresentFrame();
}

void PhxEditor::TEST_RotateEntity(float deltaTime, phx::TransformComponent& comp)
{
	using namespace DirectX;

	static std::default_random_engine s_rng;
	static std::uniform_real_distribution<float> axisDist(-1.0f, 1.0f);       // For x/y/z axis
	static std::uniform_real_distribution<float> factorDist(-1.0f, 1.0f);     // For random sign/scale
	// You can define a max angular speed (in radians per second)
	constexpr static float MAX_ANGULAR_SPEED = XM_PIDIV4; // 45 degrees/sec

	// Generate a random axis
	XMVECTOR axis = XMVectorSet(
		axisDist(s_rng),
		axisDist(s_rng),
		axisDist(s_rng),
		0.0f
	);
	axis = XMVector3Normalize(axis);

	// Random scale factor for the rotation angle
	float randomFactor = factorDist(s_rng); // Range [-1, 1]

	// Calculate final angle based on deltaTime
	float angle = randomFactor * MAX_ANGULAR_SPEED * deltaTime;

	// Delta rotation quaternion
	XMVECTOR deltaRotation = XMQuaternionRotationAxis(axis, angle);

	XMVECTOR rotationQuat = DirectX::XMLoadFloat4(&comp.Rotation);

	// Accumulate rotation
	rotationQuat = XMQuaternionNormalize(XMQuaternionMultiply(rotationQuat, deltaRotation));

	DirectX::XMStoreFloat4(&comp.Rotation, rotationQuat);
	DirectX::XMStoreFloat4x4(&comp.WorldMatrix, comp.GetMatrix());
}
