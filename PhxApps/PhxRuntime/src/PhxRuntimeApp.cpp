
#include <PhxCore/Base.h>
#include <PhxCore/SystemTime.h>
#include <PhxCore/Profiler.h>
#include <PhxCore/IO/FileUtils.h>
#include <PhxCore/IVirtualFileSystem.h>
#include <PhxCore/Memory/IAllocator.h>

#include <PhxRhi/PhxRhi.h>
#include <PhxRhi/IResourceManager.h>

#include <PhxRenderer/PhxRenderer.h>

#include <PhxResource/ResourceSystem.h>

#include <PhxWorld/GltfPrefabHandler.h>
#include <PhxEngine/EntryPoint.h>

#include <Generated/GlobalVariables.h>

using namespace phx;

class PhxRuntime final : public phx::IApplication
{
public:
	static PhxRuntime* Instance() { return ms_instance; }

public:
	PhxRuntime(const phx::ApplicationDescriptor& desc)
		: m_desc(desc)
	{
		ms_instance = this;
	}

	~PhxRuntime() { ms_instance = nullptr; }

	void Startup() override;
	void Shutdown() override;

	void OnPreRender() override;
	void OnUpdate_Threaded(float deltaTime, IAllocator* frame_allocator) override;
	void OnRender_Threaded(IAllocator* frame_allocator) override;

	const char* GetName() const override { return this->m_desc.Name.c_str(); }
	void GetDefaultWindowSize(uint32_t& outWidth, uint32_t& outHeight) const override
	{
		outWidth = m_desc.Width;
		outHeight = m_desc.Height;
	}

	void SetWindowHandle(void* handle) override
	{ 
		m_window_handle = handle; 
	}

	void* GetWindowHandle() const override { return m_window_handle; }

private:

private:
	inline static PhxRuntime* ms_instance = nullptr;
	const phx::ApplicationDescriptor m_desc;
	phx::rhi::SwapchainHandle m_swapchain = {};
	void* m_window_handle;
	phx::RefCountPtr<phx::Resource> m_box_prefab;
	
};

phx::IApplication* phx::CreateApplication()
{
	ApplicationDescriptor desc = {
		.Name = "PhxRuntime",
		.WorkingDirectory = phx::GetDirectoryWithExecutable()
	};

	return new PhxRuntime(desc);
}

void phx::DeleteApplication(phx::IApplication* ptr)
{
	delete ptr;
}

void PhxRuntime::Startup()
{
	{
		auto vfs = phx::IVirtualFileSystem::Ptr;
		vfs->Mount("res://", phx::GlobalPaths::DefaultProjectDir);
		vfs->Mount("art://", phx::GlobalPaths::ArtSrcDirectory);
		vfs->Mount("res_embedded://", "embedded://");
		// TODO: TRY mounting a pack
	}

	auto resource_system = phx::ResourceSystem::Ptr;
	resource_system->RegisterFileHanlder<phx::GltfPrefabHandler>();

	renderer::ShaderLibraryDescriptor shader_librar_desc = {
		.target = rhi::IBackend::Ptr->GetShaderFormat(),
		.include_paths = { "art://shaders/"},
		.defines = {},
#if PHX_DEBUG
		.debug_info = true,
#endif
	};

	renderer::Initialize(shader_librar_desc);

	phx::RefCountPtr<renderer::ShaderAsset> shader_asset = 
		renderer::ShaderLibrary::Ptr->LoadShader({
			.source_file_path = "art://shaders/unlit.slang",
		});

	uint32_t win_height, win_width;
	GetDefaultWindowSize(win_width, win_height);

	m_swapchain = phx::rhi::IResourceManager::Ptr->CreateSwapchain({
		.Width = win_width,
		.Height = win_height,
	});

	const char* box_prefab_path = "art://samples/box_vertex_colour/BoxVertexColors.gltf";
	PHX_INFO("Loading Test Resources '{0}'", box_prefab_path);
	m_box_prefab = resource_system->Get(box_prefab_path);
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

void PhxRuntime::Shutdown()
{
	phx::renderer::Shutdown();

	phx::rhi::IResourceManager::Ptr->DeleteSwapchain(m_swapchain);
}

void PhxRuntime::OnPreRender()
{
#if false
	PHX_PROFILE;
	{
		PHX_PROFILE_SECTION("Construct View");

		auto cameras_view = m_world.GetAllEntitiesWith<phx::NameComponent, phx::CameraComponent>();
		for (auto e : cameras_view)
		{
			auto [name_comp, camera_comp] = cameras_view.get<phx::NameComponent, phx::CameraComponent>(e);

			if (camera_comp.active)
			{
				m_per_frame_cache.num_views++;
			}
		}

		m_per_frame_cache.views = phx_new_frame View[m_per_frame_cache.num_views];

		size_t i_view = 0;
		for (auto e : cameras_view)
		{
			auto [name_comp, camera_comp] = cameras_view.get<phx::NameComponent, phx::CameraComponent>(e);

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
				auto [mesh_comp, transform_comp] = drawable_view.get<phx::MeshComponent, phx::TransformComponent>(e);
				if (!mesh_comp.Mesh->IsLoaded())
					continue;

				draw_data->num_drawables++;
			}

			draw_data->drawables = phx_new_frame Drawable[draw_data->num_drawables];

			size_t i_drawable = 0;
			for (auto e : drawable_view)
			{
				auto [mesh_comp, transform_comp] = drawable_view.get<phx::MeshComponent, phx::TransformComponent>(e);
				if (!mesh_comp.Mesh->IsLoaded())
					continue;

				Drawable& drawable = draw_data->drawables[i_drawable++];
				drawable.mesh_resource = mesh_comp.Mesh.As<phx::renderer::MeshResource>();
			}

			m_per_frame_cache.cached_data = phx_new_frame void*[1];
			*m_per_frame_cache.cached_data = draw_data;
		}
	}
#endif
}

void PhxRuntime::OnUpdate_Threaded(float /*delta_time*/, IAllocator* /*frame_allocator*/)
{
#if false
	PHX_PROFILE;

	if (m_scene_blueprint->state == phx::data::Asset::State::Loaded)
	{
		m_world.InstantiateFrom(*m_scene_blueprint);
	}
	// Rotate cube in a random direction
#endif
}

void PhxRuntime::OnRender_Threaded(IAllocator* frame_allocator)
{
	PHX_PROFILE;
	static std::atomic_uint32_t _thread_counter;

	uint32_t c = _thread_counter.fetch_add(1);
	PHX_ASSERT(c == 0);

	rhi::ISubmissionManager* submit_manager = rhi::ISubmissionManager::Ptr;
	submit_manager->BeginFrame(m_swapchain);

	rhi::ICommandBuffer* command_buffer = submit_manager->BeginCommandBuffer(rhi::CommandQueueType::Graphics);

	// -- this should be in the pre stage stage ---

	static constexpr size_t MAX_NUM_TRANSISIONS_PER_FRAME = 50;
	SpanMutable transitions = AllocateArray<GpuTransitionWork>(frame_allocator, MAX_NUM_TRANSISIONS_PER_FRAME);
	size_t num_transitions = 0;

	if (m_box_prefab->state == Resource::State::Loaded)
	{
		// this doesn't see right - Not sure we kno it's a handle resource
		auto prefab = static_cast<PrefabHandleResource*>(m_box_prefab.Get());
		for (const auto& node : prefab->prefab->nodes)
		{
			if (auto* mesh_node = std::get_if<MeshNodeData>(&node.data))
			{
				if (mesh_node->mesh->state == Resource::State::On_Gpu)
				{
					mesh_node->mesh->CollectPendingGpuTransitions(transitions, num_transitions);
				}
			}
		}
	}

	if (num_transitions)
	{
		SpanMutable<rhi::GpuBarrier> barriers = AllocateArray<rhi::GpuBarrier>(frame_allocator, num_transitions);
		for (size_t i = 0; i < num_transitions; i++)
		{
			if (auto* buffer_work = std::get_if<GpuTransitionWork::BufferWork>(&transitions[i].Data))
			{
				rhi::GpuBarrier::BufferBarrier barrier = {
					.buffer = buffer_work->buffer,
					.before_state =  rhi::ResourceStates::CopyDest,
					.after_state = buffer_work->state,
					.offset = buffer_work->offset,
					.size = buffer_work->size
				};

				barriers[i].Data = barrier;
			}
			else
			{
				PHX_ASSERT(false);
			}
		}

		command_buffer->InsertBarriers(barriers);
	}

	// -- End Caching
	command_buffer->BeginRendering(m_swapchain, { .Colour = rhi::Color(0.0f, 0.0f, 0.0f, 1.0f) });
	command_buffer->EndRendering();

	command_buffer->InsertSwapchainBarrier(m_swapchain, rhi::ResourceStates::Present);

	submit_manager->EndFrame(m_swapchain, { command_buffer });

	_thread_counter.fetch_sub(1);
#if false

	phx::rhi::CommandBufferHandle cmd_buffer = phx::rhi::BeginFrameCommandBuffer();
	ForwardPassDrawData* pass_data = static_cast<ForwardPassDrawData*>(m_per_frame_cache.cached_data[0]);
	for (size_t i = 0; i < m_per_frame_cache.num_views; i++)
	{
		//View& view = m_per_frame_cache.views[i];

		for (size_t i_drawable = 0; i_drawable < pass_data->num_drawables; i_drawable++)
		{
			Drawable& drawable = pass_data->drawables[i_drawable];

			// Bind Buffer data
			for (auto& draw_info : drawable.mesh_resource->cpu_data->Draw)
			{
				phx::rhi::CommandRecorder::DrawIndexed(
					cmd_buffer,
					draw_info.IndexCount,
					draw_info.StartIndex,
					draw_info.BaseVertex);
			}
		}
	}

	phx::rhi::SubmitAndPresentFrame();
#endif
}

#if false
void PhxRuntime::TEST_RotateEntity(float deltaTime, phx::TransformComponent& comp)
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
#endif
