
#include <PhxCore/Base.h>
#include <PhxCore/SystemTime.h>
#include <PhxCore/Profiler.h>
#include <PhxCore/IO/FileUtils.h>
#include <PhxCore/IVirtualFileSystem.h>
#include <PhxCore/Memory/IAllocator.h>

#include <PhxRhi/PhxRhi.h>
#include <PhxRhi/IResourceManager.h>

#include <PhxRenderer/PhxRenderer.h>
#include <PhxRenderer/MeshResource.h>

#include <PhxResource/ResourceSystem.h>

#include <PhxWorld/GltfPrefabHandler.h>
#include <PhxWorld/World.h>
#include <PhxWorld/Entity.h>

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

	void OnPreRender(IAllocator* frame_allocator) override;
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
	void ProcessSpawnRequests();

	// potential shader/material functions
	rhi::PipelineStateHandle CreateTestPso(const renderer::ShaderAsset& shader_asset);

	// Potential renderer functions
	void Renderer_RecordTransitions(rhi::ICommandBuffer* command_buffer, Span<GpuTransitionWork> transisions);

private:
	inline static PhxRuntime* ms_instance = nullptr;
	const phx::ApplicationDescriptor m_desc;
	phx::rhi::SwapchainHandle m_swapchain = {};
	void* m_window_handle;
	
	// -- Prototyping memebers ---
	phx::RefCountPtr<renderer::ShaderAsset> m_test_shader;
	rhi::PipelineStateHandle m_test_pso;

	// TODO: Move some stuff into the application level that don't need to be global.
	// Example, renderer, shader libary, material system etc.
	World m_world;
	std::vector<phx::RefCountPtr<phx::Resource>> m_spawn_requests;

	struct alignas(64) RenderPacket
	{
		uint64_t sort_key;

		rhi::PipelineStateHandle pso;
		rhi::BufferHandle packed_buffer;

		uint64_t index_buffer_address;
		uint64_t vertex_buffer_address;

		// --- 24 Bytes: Draw Args ---
		uint32_t index_count;
		uint32_t first_index;
		int32_t  vertex_offset; // standard "baseVertex"
#if false
		uint32_t instance_index;
#else
		hlslpp::float4x4 world_matrix;
#endif

		// --- 16 Bytes: Push Constants ---
		uint32_t material_id;   // Index into Global Material Buffer
	};
#if false
	static_assert(sizeof(RenderPacket) == 64);
#else
	static_assert(sizeof(RenderPacket) == 128);
#endif

	GpuTransitionWork* m_render_transitions;
	size_t m_num_render_transitions;
	RenderPacket* m_render_packets;
	size_t m_num_render_packets;
	
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

	m_test_shader = renderer::ShaderLibrary::Ptr->LoadShader({
			.source_file_path = "art://shaders/unlit.slang",
			.entry_points = {
				{ .name = "VertexMain",		.stage = rhi::ShaderStage::VS },
				{ .name = "FragmentMain",	.stage = rhi::ShaderStage::PS }
			}
		});

	m_test_pso = CreateTestPso(*m_test_shader);

	uint32_t win_height, win_width;
	GetDefaultWindowSize(win_width, win_height);

	m_swapchain = phx::rhi::IResourceManager::Ptr->CreateSwapchain({
		.Width = win_width,
		.Height = win_height,
	});

	const char* box_prefab_path = "art://samples/box_vertex_colour/BoxVertexColors.gltf";
	PHX_INFO("Loading Test Resources '{0}'", box_prefab_path);
	m_spawn_requests.push_back(resource_system->Get(box_prefab_path));

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

	auto rm = phx::rhi::IResourceManager::Ptr;
	rm->DeleteSwapchain(m_swapchain);
	if (m_test_pso.IsValid())
		rm->DeletePipeline(m_test_pso);

}

void PhxRuntime::OnPreRender(IAllocator* frame_allocator)
{
	auto group = m_world.GetRegistry().group<StaticMeshComponent>(entt::get<WorldTransformComponent>);
	const size_t max_num_packets = group.size();

	static constexpr size_t MAX_NUM_PER_MESH_DRAWS = 6;
	m_render_packets = AllocateArray<RenderPacket>(frame_allocator, max_num_packets * MAX_NUM_PER_MESH_DRAWS).data();
	m_num_render_packets = 0;

	static constexpr size_t MAX_NUM_TRANSISIONS_PER_FRAME = 50;
	m_render_transitions = AllocateArray<GpuTransitionWork>(frame_allocator, MAX_NUM_TRANSISIONS_PER_FRAME).data();
	m_num_render_transitions = 0;

	for (auto entity : group)
	{
		const auto& mesh_component				= group.get<StaticMeshComponent>(entity);
		const auto& world_transform_component	= group.get<WorldTransformComponent>(entity);

		renderer::MeshResource* mesh_resource = static_cast<renderer::MeshResource*>(mesh_component.mesh);
		if (m_num_render_transitions < MAX_NUM_TRANSISIONS_PER_FRAME && mesh_resource->state == Resource::State::On_Gpu)
		{
			mesh_resource->CollectPendingGpuTransitions({ m_render_transitions, MAX_NUM_TRANSISIONS_PER_FRAME }, m_num_render_transitions);
			mesh_resource->state = Resource::State::Loaded;
		}

		uint64_t packed_buffer_address = rhi::IResourceManager::Ptr->GetGpuAddress(mesh_resource->packed_mesh_buffer);
		TypedView<renderer::MeshResource::CpuData>& cpu_data = mesh_resource->cpu_data;

		// TODO: Validate against materials
		uint8_t draw_count = (uint8_t)cpu_data->num_draws;
		if (draw_count >= MAX_NUM_PER_MESH_DRAWS)
		{
			auto& name_component = m_world.GetRegistry().get<NameComponent>(entity);
			PHX_WARN(
				"Mesh instance {0} has {1} draws. Which exceeds upper limit of {2}",
				name_component.Name.c_str(),
				draw_count,
				MAX_NUM_PER_MESH_DRAWS);
		}

		for (uint8_t i = 0; i < draw_count; ++i)
		{
			const auto& draw_info = cpu_data->draws[i];
			// const auto* material = mesh_component.materials[i];
			RenderPacket& packet = m_render_packets[m_num_render_packets++];

			packet.index_count = draw_info.prim_count;
			packet.first_index = draw_info.start_index;
			packet.vertex_offset = draw_info.base_vertex;

			packet.packed_buffer = mesh_resource->packed_mesh_buffer;
			packet.index_buffer_address = packed_buffer_address + cpu_data->index_data_offset;
			packet.vertex_buffer_address = packed_buffer_address + cpu_data->vertex_data_offset;

#if false // Example of how to link pso with material instance
			packet.pso = material->GetPSO(RenderPass::Forward);
#else
			packet.pso = m_test_pso;
#endif

			// TODO: Use instance ID
			packet.world_matrix = world_transform_component.world_matrix;
			packet.material_id = ~0u;

			// example_sorting
			// packet.sort_key = CalculateSortKey(0, transform.depth, material->bindless_index);
			packet.sort_key = 0ul;
		}
	}
}

void PhxRuntime::OnUpdate_Threaded(float /*delta_time*/, IAllocator* /*frame_allocator*/)
{
	using namespace hlslpp;

	{
		ProcessSpawnRequests();
	}

	// -- Update world transforms ---
	// TODO: Handle parent logic here as well
	// TODO: Profile if group is better then view here.
	const auto& view = m_world.GetRegistry().view<TransformComponent>();
	for (auto entity : view)
	{
		auto transform = view.get<TransformComponent>(entity);
		if (!transform.IsDirty())
			continue;

		const float4x4 rotation_matrix(transform.rotation);
		const float4x4 scale_matrix = float4x4::scale(transform.scale);
		const float4x4 translation_matrix = float4x4::translation(transform.translation);

		auto& world_transform = m_world.GetRegistry().emplace_or_replace<WorldTransformComponent>(entity);
		world_transform.world_matrix = hlslpp::float4x4::identity();
		world_transform.world_matrix = translation_matrix * rotation_matrix * scale_matrix;
	}
}

void PhxRuntime::OnRender_Threaded(IAllocator* /*frame_allocator*/)
{
	PHX_PROFILE;
	static std::atomic_uint32_t _thread_counter;

	uint32_t c = _thread_counter.fetch_add(1);
	PHX_ASSERT(c == 0);

	rhi::ISubmissionManager* submit_manager = rhi::ISubmissionManager::Ptr;
	submit_manager->BeginFrame(m_swapchain);

	rhi::ICommandBuffer* command_buffer = submit_manager->BeginCommandBuffer(rhi::CommandQueueType::Graphics);

	// -- Process pending transisions ---
	if (m_num_render_transitions)
	{
		Renderer_RecordTransitions(command_buffer, Span(m_render_transitions, m_num_render_transitions));
	}

	// -- End Caching
	command_buffer->BeginRendering(m_swapchain, { .Colour = rhi::Color(0.0f, 0.0f, 0.0f, 1.0f) });

	// Render Packets

	command_buffer->EndRendering();

	command_buffer->InsertSwapchainBarrier(m_swapchain, rhi::ResourceStates::Present);

	submit_manager->EndFrame(m_swapchain, { command_buffer });

	_thread_counter.fetch_sub(1);
}

void PhxRuntime::ProcessSpawnRequests()
{
	CpuTimer spawn_timer;

	for (size_t i = m_spawn_requests.size() - 1; i >= 0; --i)
	{
		if (spawn_timer.Elapsed().GetMilliseconds() >= 2)
		{
			PHX_INFO("Spawner excited allocated frame time of 2MS. Deferring spawns until next frame");
			break;
		}

		RefCountPtr<Resource>& resource = m_spawn_requests[i];

		if (!resource->IsLoaded())
			continue;

		if (resource->type_id != PrefabHandleResource::StaticTypeId())
			continue;

		auto prefab = static_cast<PrefabHandleResource*>(resource.Get())->prefab;

		PHX_INFO("Spawning prefab into world.");
		for (const auto& node : prefab->nodes)
		{
			Entity entity = m_world.CreateEntity(node.name);
			auto& transform_component = entity.GetComponent<TransformComponent>();

			// TODO: Collapse transforms and link parents
			transform_component.scale = node.scale;
			transform_component.rotation = node.rotation;
			transform_component.translation = node.translation;
			transform_component.dirty = true;

			std::visit([&](auto&& target) {
				using TTarget = std::decay_t<decltype(target)>;

				if constexpr (std::is_same_v<TTarget, MeshNodeData>)
				{
					const MeshNodeData& mesh_node_data = target;
					auto& static_mesh_component = entity.AddComponent<StaticMeshComponent>();
					auto& storage_component = entity.AddComponent<StaticMeshStorageComponent>();

					storage_component.mesh = mesh_node_data.mesh;
					static_mesh_component.mesh = mesh_node_data.mesh.Get();

					// TODO: implement material system.
					// storage_component.materials[0] = mesh_node_data.material;
#if false
					static_mesh_component.materials[0] = mesh_node_data.material.Get();
					static_mesh_component.num_materials = 1;
#endif
				}
			}, node.data);

		}
		m_spawn_requests[i] = std::move(m_spawn_requests.back());
		m_spawn_requests.pop_back();
	}
}

rhi::PipelineStateHandle PhxRuntime::CreateTestPso(const renderer::ShaderAsset& shader_asset)
{
	const RefCountPtr<renderer::SlangShader>& shader = shader_asset.Get();
	rhi::PipelineStateDescriptor desc = {
		.shader_stages = {
			rhi::ShaderStageInfo{ 
				.stage = rhi::ShaderStage::VS,
				.module_handle = shader->GetShaderModule(),
				.entry_point = shader->GetEntryPoint(rhi::ShaderStage::VS)},
			rhi::ShaderStageInfo{
				.stage = rhi::ShaderStage::PS,
				.module_handle = shader->GetShaderModule(),
				.entry_point = shader->GetEntryPoint(rhi::ShaderStage::PS)},
		},
		.render_pass_info = {
			.color_attachments = { rhi::Format::RGBA16_FLOAT },
			.depth_stencil_format = rhi::Format::D32,
		}
	};

	return rhi::IResourceManager::Ptr->CreatePipeline(desc);
}

void PhxRuntime::Renderer_RecordTransitions(rhi::ICommandBuffer* command_buffer, Span<GpuTransitionWork> transitions)
{
	StaticArray<rhi::GpuBarrier, 16> barriers;
	uint32_t batch_count = 0;

	for (uint32_t i = 0; i < transitions.size(); ++i)
	{
		if (auto* buffer_work = std::get_if<GpuTransitionWork::BufferWork>(&transitions[i].Data))
		{
			// 2. Create the barrier data
			rhi::GpuBarrier::BufferBarrier barrier_data = {
				.buffer = buffer_work->buffer,
				.before_state = rhi::ResourceStates::CopyDest,
				.after_state = buffer_work->state,
				.offset = buffer_work->offset,
				.size = buffer_work->size
			};

			barriers[batch_count].Data = barrier_data;
			batch_count++;
		}
		else
		{
			PHX_ASSERT(false);
		}

		// 4. If the batch is full, submit and reset
		if (batch_count == 16)
		{
			// If your StaticArray has a '.count' member, make sure to update it!
			// barriers.count = 16; 

			command_buffer->InsertBarriers(barriers);

			// Reset counter for the next batch
			batch_count = 0;
		}
	}

	if (batch_count > 0)
	{
		command_buffer->InsertBarriers(Span(barriers.begin(), batch_count));
	}
}
