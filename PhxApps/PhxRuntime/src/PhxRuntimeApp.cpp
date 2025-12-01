
#include <PhxCore/Base.h>
#include <PhxCore/SystemTime.h>
#include <PhxCore/Profiler.h>
#include <PhxCore/IO/FileUtils.h>
#include <PhxCore/IVirtualFileSystem.h>
#include <PhxCore/Memory/IAllocator.h>

#include <PhxRhi/PhxRhi.h>

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
	void Renderer_RecordTransitions(rhi::CmdHandle command_buffer, Span<GpuTransitionWork> transisions);

private:
	inline static PhxRuntime* ms_instance = nullptr;
	const phx::ApplicationDescriptor m_desc;
	phx::rhi::SwapchainHandle m_swapchain = {};
	void* m_window_handle;
	
	// -- Prototyping memebers ---
	phx::RefCountPtr<renderer::ShaderAsset> m_test_shader;
	rhi::PipelineStateHandle m_test_pso;
	std::vector<rhi::TextureHandle> m_depth_textures;

	// TODO: Move some stuff into the application level that don't need to be global.
	// Example, renderer, shader libary, material system etc.
	World m_world;
	std::vector<phx::RefCountPtr<phx::Resource>> m_spawn_requests;


	/*
	[Byte 0  - 8  ] Sort Key
	[Byte 8  - 12 ] PSO Handle
	[Byte 12 - 16 ] Packed Buffer Handle
	[Byte 16 - 24 ] Index Buffer Address (64-bit)
	[Byte 24 - 40 ] Draw Args (IndexCount, FirstIndex, VertexOffset, Instance)
	[Byte 40 - 48 ] Padding (Empty space)
	-----------------------------------------------------------------------
	[Byte 48 - 112] Model Matrix (64 Bytes)   <-- START PUSH CONSTANTS
	[Byte 112- 116] Material ID
	[Byte 116- 120] Padding (Alignment for address)
	[Byte 120- 128] Vertex Buffer Address     <-- END PUSH CONSTANTS
	*/
	struct alignas(64) RenderPacket
	{
		uint64_t sort_key;
		rhi::PipelineStateHandle pso;
		rhi::BufferHandle packed_buffer;

		// --- 24 Bytes: Draw Args ---
		uint32_t index_count;
		uint32_t first_index;
		int32_t  vertex_offset; // standard "baseVertex"
		
		std::byte _padding[8];
		struct PushConstants
		{
#if false
			uint32_t instance_index;
#else
			hlslpp::float4x4 world_matrix;
#endif

			// --- 16 Bytes: Push Constants ---
			uint32_t material_id;   // Index into Global Material Buffer
			std::byte __padding[4];
			uint64_t vertex_buffer_address;
		} push_constants;
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
	PHX_INFO("Runtime Application starting up");
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
		.target = rhi::GetShaderFormat(),
		.include_paths = { "art://shaders/"},
		.defines = {},
#if PHX_DEBUG
		.debug_info = true,
#endif
	};

	PHX_INFO("Initializing Shader Library");
	renderer::Initialize(shader_librar_desc);

	PHX_INFO("Loading test shader. 'art://shaders/cube_validate_raw.slang'");
	m_test_shader = renderer::ShaderLibrary::Ptr->LoadShader({
			.source_file_path = "art://shaders/cube_validate_raw.slang",
			.entry_points = {
				{ .name = "VertexMain",		.stage = rhi::ShaderStage::VS },
				{ .name = "FragmentMain",	.stage = rhi::ShaderStage::PS }
			}
		});

	PHX_INFO("Creating test PSO");
	m_test_pso = CreateTestPso(*m_test_shader);

	uint32_t win_height, win_width;
	GetDefaultWindowSize(win_width, win_height);

	PHX_INFO("Creating Swapchain w={0},h={1}", win_width, win_height);
	m_swapchain = phx::rhi::CreateSwapchain({
		.Width = win_width,
		.Height = win_height,
	});

	const uint32_t num_swapchain_images = rhi::GetSwapchainImageCount(m_swapchain);
	m_depth_textures.resize(num_swapchain_images);

	PHX_INFO("Creating {0} depth textures", num_swapchain_images);
	for (size_t i = 0; i < num_swapchain_images; ++i)
	{
		m_depth_textures[i] = rhi::CreateTexture({
			.DebugName = "Depth Texture",
			.Format = rhi::Format::D32,
			.Width = win_width,
			.Height = win_height,
			.BindingFlags = rhi::BindingFlags::DepthStencil
		});

		// Push Back Transition
	}

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

	for (size_t i = 0; i < m_depth_textures.size(); ++i)
	{
		rhi::DeleteTexture(m_depth_textures[i]);
	}

	phx::rhi::DeleteSwapchain(m_swapchain);
	if (m_test_pso.IsValid())
		phx::rhi::DeletePipeline(m_test_pso);

}

void PhxRuntime::OnPreRender(IAllocator* frame_allocator)
{
	auto group = m_world.GetRegistry().group<StaticMeshComponent>(entt::get<WorldTransformComponent>);
	const size_t max_num_packets = group.size();
	m_num_render_transitions = 0;
	m_num_render_packets = 0;

	if (max_num_packets == 0)
		return;

	static constexpr size_t MAX_NUM_PER_MESH_DRAWS = 6;
	m_render_packets = AllocateArray<RenderPacket>(frame_allocator, max_num_packets * MAX_NUM_PER_MESH_DRAWS).data();

	static constexpr size_t MAX_NUM_TRANSISIONS_PER_FRAME = 50;
	m_render_transitions = AllocateArray<GpuTransitionWork>(frame_allocator, MAX_NUM_TRANSISIONS_PER_FRAME).data();

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

		if (mesh_resource->state != Resource::State::Loaded)
			continue;

		uint64_t packed_buffer_address = phx::rhi::GetGpuAddress(mesh_resource->packed_mesh_buffer);
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
			//packet.index_buffer_address = packed_buffer_address + cpu_data->index_data_offset;

			packet.push_constants.vertex_buffer_address = packed_buffer_address + cpu_data->vertex_data_offset;

#if false // Example of how to link pso with material instance
			packet.pso = material->GetPSO(RenderPass::Forward);
#else
			packet.pso = m_test_pso;
#endif

			// TODO: Use instance ID
			packet.push_constants.world_matrix = world_transform_component.world_matrix;
			packet.push_constants.material_id = ~0u;

			// example_sorting
			// packet.sort_key = CalculateSortKey(0, transform.depth, material->bindless_index);
			packet.sort_key = 0ul;
		}
	}
}

void PhxRuntime::OnUpdate_Threaded(float delta_time, IAllocator* /*frame_allocator*/)
{
	using namespace hlslpp;

	{
		ProcessSpawnRequests();
	}

	// -- Rotation Logic ---

	const auto& view = m_world.GetRegistry().view<TransformComponent>();
	(void)delta_time;
#if false
	const float rotation_speed = 0.5f;
	const float t = (float)SystemTime::GetCurrentTick() * 0.001f;
	for (auto entity : view)
	{
		float3 tumble_axis = float3(
			sin(t),
			cos(t * 0.5f),
			0.5f
		);

		// [CRITICAL FIX] Normalize the axis or the mesh will distort/skew!
		tumble_axis = hlslpp::normalize(tumble_axis);

		auto& transform = view.get<TransformComponent>(entity);

		quaternion delta_rot = quaternion::rotation_axis(tumble_axis, rotation_speed * delta_time);
		transform.rotation = hlslpp::normalize(hlslpp::mul(transform.rotation, delta_rot));
		transform.dirty = true;
	}
#endif
	// -- LOOP 2: UPDATE MATRICES ---
	for (auto entity : view)
	{
		auto& transform = view.get<TransformComponent>(entity);

		if (!transform.IsDirty())
			continue;

		const float4x4 rotation_matrix = float4x4(transform.rotation);
		const float4x4 scale_matrix = float4x4::scale(transform.scale);
		const float4x4 translation_matrix = float4x4::translation(transform.translation);

		auto& world_transform = m_world.GetRegistry().emplace_or_replace<WorldTransformComponent>(entity);
		world_transform.world_matrix = translation_matrix * rotation_matrix * scale_matrix;
		transform.dirty = false;
	}
}

void PhxRuntime::OnRender_Threaded(IAllocator* /*frame_allocator*/)
{
	PHX_PROFILE;
	static std::atomic_uint32_t _thread_counter;

	uint32_t c = _thread_counter.fetch_add(1);
	PHX_ASSERT(c == 0);

	phx::rhi::BeginFrame(m_swapchain);

	rhi::CmdHandle command_buffer = phx::rhi::BeginCommandBuffer(rhi::CommandQueueType::Graphics);

	// -- Process pending transisions ---
	if (m_num_render_transitions)
	{
		Renderer_RecordTransitions(command_buffer, Span(m_render_transitions, m_num_render_transitions));
	}

	// -- End Caching
	uint32_t current_image_index = rhi::GetSwapchainImageIndex(m_swapchain);
	rhi::BeginRendering(
		command_buffer,
		m_swapchain,
		{ .Colour = rhi::Color(0.0f, 0.0f, 0.0f, 1.0f)},
		m_depth_textures[current_image_index],
		{ .DepthStencil = {.Depth = 1.0f, .Stencil = 0 } });

	uint32_t w, h;
	GetDefaultWindowSize(w, h);

	// -- TEST --
	static const hlslpp::float3 cam_pos = hlslpp::float3(0.0f, 2.0f, -4.0f);
	static const hlslpp::float3 cam_target = hlslpp::float3(0.0f, 0.0f, 0.0f);
	static const hlslpp::float3 cam_up = hlslpp::float3(0.0f, 1.0f, 0.0f);

	const hlslpp::float4x4 view = hlslpp::float4x4::look_at(cam_pos, cam_target, cam_up);

	// 1. Setup Window & Camera Data
	float aspect_ratio = w / h;

	float fov_radians = 1.047f; // ~60 degrees
	float near_z = 0.1f;
	float far_z = 1000.0f;

	hlslpp::frustum f = hlslpp::frustum::field_of_view_y(
		fov_radians,
		aspect_ratio,
		near_z,
		far_z
	);
	const hlslpp::projection p(f, hlslpp::zclip::zero);

	// Note: hlslpp projection matrices map Z to [0, 1] by default
	const hlslpp::float4x4 proj = hlslpp::float4x4::perspective(p);

	struct PushConstants
	{
		hlslpp::float4x4 mvp;
		hlslpp::float4x4 model_matrix;
		uint64_t vertex_buffer_ptr;
	};
	// -- END TEST

	rhi::Viewport vp(w, h);
	rhi::Rect rect(w, h);
	for (size_t i_render_packet = 0; i_render_packet < m_num_render_packets; ++i_render_packet)
	{
		const RenderPacket& render_packet = m_render_packets[i_render_packet];
	
		rhi::BindPipelineState(command_buffer, render_packet.pso);
		{
			rhi::SetViewport(command_buffer, vp);
			rhi::SetScissor(command_buffer, rect);
			rhi::SetPrimitiveTopology(command_buffer, rhi::PrimitiveType::TriangleList);
			rhi::SetDepthTest(command_buffer, true, true, rhi::ComparisonFunc::Less);
			rhi::SetStencilTest(command_buffer, false);
			rhi::SetCullMode(command_buffer, rhi::RasterCullMode::Back);
			rhi::SetFrontFace(command_buffer, rhi::FrontFace::CounterClockwise);
		}

		rhi::BindIndexBuffer(command_buffer, render_packet.packed_buffer, 0ul);
		PushConstants push = {
			.mvp = mul(mul(render_packet.push_constants.world_matrix, view), proj),
			.model_matrix = render_packet.push_constants.world_matrix,
			.vertex_buffer_ptr = render_packet.push_constants.vertex_buffer_address,
		};

		rhi::PushConstants(command_buffer, &push, sizeof(PushConstants));

		rhi::DrawIndexed(command_buffer, render_packet.index_count, render_packet.first_index, 0);
	}

	rhi::EndRendering(command_buffer);

	rhi::InsertSwapchainBarrier(command_buffer, m_swapchain, rhi::ResourceStates::Present);

	rhi::EndFrame(m_swapchain, { command_buffer });

	_thread_counter.fetch_sub(1);
}

void PhxRuntime::ProcessSpawnRequests()
{
	CpuTimer spawn_timer;

	for (int i = (int)m_spawn_requests.size() - 1; i >= 0; --i)
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
		// swap and pop to delete the procedded request.
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
			.color_attachments = { rhi::Format::SBGRA8_UNORM },
			.depth_stencil_format = rhi::Format::D32,
		}
	};

	return rhi::CreatePipeline(desc);
}

void PhxRuntime::Renderer_RecordTransitions(rhi::CmdHandle command_buffer, Span<GpuTransitionWork> transitions)
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

			rhi::InsertBarriers(command_buffer, barriers);

			// Reset counter for the next batch
			batch_count = 0;
		}
	}

	if (batch_count > 0)
	{
		rhi::InsertBarriers(command_buffer, Span(barriers.begin(), batch_count));
	}
}
