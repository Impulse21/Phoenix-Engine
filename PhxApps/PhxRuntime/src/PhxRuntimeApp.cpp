
#if true // This section is to test a little refactor with core engine. Mostly due to changes in platform support

#include <PhxCore/Base.h>
#include <PhxCore/SystemTime.h>
#include <PhxCore/Profiler.h>
#include <PhxCore/IO/FileUtils.h>
#include <PhxCore/IVirtualFileSystem.h>
#include <PhxCore/Memory/IAllocator.h>
#include <PhxCore/Platform/PlatformWindow.h>

#include <PhxResource/ResourceManager.h>
#include <PhxAsset/AssetDatabase.h>

#include <PhxWorld/GltfPrefabHandler.h>

#include <PhxEngine/Application.h>
#include <PhxEngine/EntryPoint.h>

#include "GlobalPaths.h"

// -- TEMP ---
#include <PhxCore/Reflect/TypeInfo.h>
#include <PhxRenderer/MaterialArchetype.def.h>

constexpr bool SET_FORCE_RECOOK = false;

class PhxRuntime final : public phx::IApplication
{
public:
	static PhxRuntime* Instance() { return ms_instance; }

public:
	PhxRuntime()
	{
		ms_instance = this;
	}

	~PhxRuntime() { ms_instance = nullptr; }


	void ConfigureServices(phx::EngineServices& /*services*/) override {};
	void ConfigureWindow(phx::WindowDescriptor& win_desc) override
	{
		win_desc.title = GetName();
		win_desc.width = 1600;
		win_desc.height = 900;
		win_desc.flags.VSync = false;
		win_desc.flags.FullScreen = false;
	}

	bool Startup(const phx::EngineContext& engine_context) override;
	void Shutdown() override;

	void OnPreRender(phx::IAllocator* frame_allocator) override;
	void OnUpdate_Threaded(float deltaTime, phx::IAllocator* frame_allocator) override;
	void OnRender_Threaded(phx::IAllocator* frame_allocator) override;

	const char* GetName() const override 
	{
		 constexpr const char* app_name = "Phoenix Runtime";
		  return app_name; 
	}

private:
	inline static PhxRuntime* ms_instance = nullptr;
	phx::EngineContext m_engine_context;

	phx::rhi::SwapchainHandle m_swapchain = {};
};

phx::IApplication* phx::CreateApplication()
{
	return new PhxRuntime();
}

bool PhxRuntime::Startup(const phx::EngineContext& engine_context)
{
	using namespace phx;

	m_engine_context = engine_context;

	PHX_INFO("Runtime Application starting up");
	{
		auto vfs = phx::IVirtualFileSystem::Ptr;
		vfs->Mount("res://", phx::GlobalPaths::DefaultProjectDir);
		vfs->Mount("art://", phx::GlobalPaths::ArtSrcDirectory);
		vfs->Mount("assets://", phx::GlobalPaths::ArtSrcDirectory);
		vfs->Mount("res_embedded://", "embedded://");
	}

	// -- TEMP CODE ----
	const std::string archetype_path = "assets://mat_arch/standard.phxmar";
	AssetPtr<renderer::assets::MaterialArchetype> standard_mtl_archetype = phx::asset::AssetDB::Get<renderer::assets::MaterialArchetype>(archetype_path);

	if (standard_mtl_archetype)
	{
		std::stringstream ss;
		ss << "Shader Name: " << standard_mtl_archetype->shader_desc.source << "\n\t\t";
		ss << "is double sided: " << standard_mtl_archetype->is_double_sided << "\n\t\t";

		std::string info_str = ss.str();

		PHX_INFO("Loaded material archetype from {0}\n\tInfo -> \n\t\t{1}", archetype_path, info_str);
	}
	else
	{
		PHX_ERROR("Failed to load material archtype from {0}", archetype_path);
	}
	// -- TEMP CODE (END) ---

	phx::EngineCore::RequestExit();
	return false;

	phx::ResourceManager::RegisterLoader<phx::GltfPrefabLoader>(".gltf");
	phx::GltfPrefabLoader::SetForceRecook(SET_FORCE_RECOOK);
	if (SET_FORCE_RECOOK)
		PHX_WARN("GltfPrefabLoader is set to FORCE RECOOK mode. All prefabs and leaf resources will be recooked on load.");

	auto [win_width, win_height] = phx::Platform::GetWindowSize(engine_context.window_handle);

	PHX_INFO("Creating Swapchain w={0},h={1}", win_width, win_height);
	m_swapchain = phx::rhi::CreateSwapchain({
		.Width = win_width,
		.Height = win_height,
	});

	return true;
}

void PhxRuntime::Shutdown()
{
	phx::rhi::WaitForIdle();

	if (m_swapchain.IsValid())
		phx::rhi::DeleteSwapchain(m_swapchain);
}


void PhxRuntime::OnPreRender(phx::IAllocator* /*frame_allocator*/)
{
}

void PhxRuntime::OnUpdate_Threaded(float /*deltaTime*/, phx::IAllocator* /*frame_allocator*/)
{
}

void PhxRuntime::OnRender_Threaded(phx::IAllocator* /*engine_context*/)
{
	phx::rhi::BeginFrame(m_swapchain);

	phx::rhi::CmdHandle command_buffer = phx::rhi::BeginCommandBuffer(phx::rhi::CommandQueueType::Graphics);

	phx::rhi::BeginRendering(
		command_buffer,
		m_swapchain,
		{ .Colour = phx::rhi::Color(0.0f, 0.0f, 0.0f, 1.0f)});
		
	phx::rhi::EndRendering(command_buffer);
	phx::rhi::InsertSwapchainBarrier(command_buffer, m_swapchain, phx::rhi::ResourceStates::Present);

	phx::rhi::EndFrame(m_swapchain, { command_buffer });
}

#else // Previous code

#include <PhxCore/Base.h>
#include <PhxCore/SystemTime.h>
#include <PhxCore/Profiler.h>
#include <PhxCore/IO/FileUtils.h>
#include <PhxCore/IVirtualFileSystem.h>
#include <PhxCore/Memory/IAllocator.h>

#include <PhxRhi/PhxRhi.h>

#include <PhxRenderer/PhxRenderer.h>
#include <PhxRenderer/MeshResource.h>
#include <PhxRenderer/MaterialResource.h>
#include <PhxRenderer/TextureResource.h>

#include <PhxResource/ResourceManager.h>

#include <PhxWorld/PrefabResource.h>
#include <PhxWorld/GltfPrefabHandler.h>
#include <PhxWorld/World.h>
#include <PhxWorld/Entity.h>

#include <PhxEngine/EntryPoint.h>

#include "GlobalPaths.h"

using namespace phx;

constexpr bool SET_FORCE_RECOOK = false;

struct MaterialField
{
	uint32_t offset = 0;
	uint32_t size = 0;
	// MaterialPropertyType type = MaterialPropertyType::Float;
};

struct MaterialArchetype
{
	RefCountPtr<renderer::ShaderAsset> shader_asset;
	std::unordered_map<std::string, MaterialField> layout_map;

	const MaterialField* GetField(const std::string& name) const
	{
		auto it = layout_map.find(name);
		if (it != layout_map.end())
		{
			return &it->second;
		}
		return nullptr;
	}

	void BuildReflectionCache()
	{
		slang::ProgramLayout* layout = shader_asset->Get()->GetReflection();
		slang::TypeReflection* mat_type = layout->findTypeByName("MaterialData");
		slang::TypeLayoutReflection* type_layout = layout->getTypeLayout(mat_type);

		// 2. Iterate Fields
		uint32_t count = mat_type->getFieldCount();
		for (uint32_t i = 0; i < count; ++i)
		{
			// VariableLayoutReflection has the offset!
			slang::VariableLayoutReflection* var_layout = type_layout->getFieldByIndex(i);
			const char* name = var_layout->getName();

			if (std::string_view(name) == "_pad") continue;

			MaterialField field{
				.offset = (uint32_t)var_layout->getOffset(SLANG_PARAMETER_CATEGORY_UNIFORM),
				.size = (uint32_t)var_layout->getTypeLayout()->getSize(SLANG_PARAMETER_CATEGORY_UNIFORM),
			};

			layout_map[name] = field;
		}
	}

	template <typename T>
	void SetProperty(std::byte* dest_ptr, const std::string& name, const T& value)
	{
		const MaterialField* field = GetField(name);
		if (!field)
		{
			PHX_CORE_WARN("Material Property '{0}' not found in shader.", name);
			return;
		}
		PHX_ASSERT(field->size == sizeof(T), "Size mismatch for property");

		dest_ptr += field->offset;
		std::memcpy(dest_ptr, &value, sizeof(T));
	}

	void SetFloat(std::byte* dest_ptr, const std::string& name, float v)
	{
		SetProperty<float>(dest_ptr, name, v);
	}

	void SetInt(std::byte* dest_ptr, const std::string& name, int32_t v)
	{
		SetProperty<int32_t>(dest_ptr, name, v);
	}

	void SetBool(std::byte* dest_ptr, const std::string& name, bool v)
	{
		SetProperty<bool>(dest_ptr, name, v);
	}

	void SetFloat2(std::byte* dest_ptr, const std::string& name, const hlslpp::interop::float2& v)
	{
		SetProperty<hlslpp::interop::float2>(dest_ptr, name, v);
	}
	void SetFloat3(std::byte* dest_ptr, const std::string& name, const hlslpp::interop::float3& v)
	{
		SetProperty<hlslpp::interop::float3>(dest_ptr, name, v);
	}
	void SetFloat4(std::byte* dest_ptr, const std::string& name, const hlslpp::interop::float4& v)
	{
		SetProperty<hlslpp::interop::float4>(dest_ptr, name, v);
	}

	void SetTexture(std::byte* dest_ptr, const std::string& name, renderer::TextureResource& texture_resource)
	{
		// 1. Resolve to Bindless ID
		uint32_t bindless_id = ~0ul; // Default White
		if (texture_resource.state == ResourceState::Loaded)
		{
			// Assuming TextureSystem helper
			bindless_id = (uint32_t)rhi::GetDescriptorIndex(texture_resource.texture_handle);
			PHX_INFO("Texture variable {0} is loaded. Setting Bindless index to {1}.", name, bindless_id);
		}
		else
		{
			PHX_WARN("Texture variable {0} isn't loaded yet.", name);
		}

		SetProperty<uint32_t>(dest_ptr, name, bindless_id);
	}
};


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
	void Renderer_RecordResourceTransitions(rhi::CmdHandle command_buffer);

private:
	inline static PhxRuntime* ms_instance = nullptr;
	const phx::ApplicationDescriptor m_desc;
	phx::rhi::SwapchainHandle m_swapchain = {};
	void* m_window_handle;
	
	// -- Prototyping memebers ---
	MaterialArchetype m_standard_archetype;

	rhi::PipelineStateHandle m_standard_pso;
	std::vector<rhi::TextureHandle> m_depth_textures;

	// TODO: Move some stuff into the application level that don't need to be global.
	// Example, renderer, shader libary, material system etc.
	World m_world;
	std::vector<PrefabResourcePtr> m_spawn_requests;
	
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

	RenderPacket* m_render_packets;
	size_t m_num_render_packets;

	uint8_t* m_render_thread_mtl_shadow_data;
	size_t m_render_thread_mtl_shadow_size;

	// -- This is only to test. These should be managed by rendering system ---
	// This is currently appened to the Dynamic Allocation every frame (Not ideal)
	std::vector<MaterialResourcePtr> m_requires_patching;
	phx::MemoryBuffer m_material_shadow_data;
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

	ResourceManager::RegisterLoader<GltfPrefabLoader>(".gltf");
	phx::GltfPrefabLoader::SetForceRecook(SET_FORCE_RECOOK);
	if (SET_FORCE_RECOOK)
		PHX_WARN("GltfPrefabLoader is set to FORCE RECOOK mode. All prefabs and leaf resources will be recooked on load.");

	renderer::ShaderLibraryDescriptor shader_librar_desc = {
		.target = rhi::GetShaderFormat(),
		.include_paths = { "art://shaders/"},
		.defines = {},
#if PHX_DEBUG
		.save_debug_symbols = true,
		.debug_info = true,
		.optimization = false,
#endif
	};

	PHX_INFO("Initializing Shader Library");
	renderer::Initialize(shader_librar_desc);

	PHX_INFO("Loading standard shader. 'art://shaders/standard.slang'");
	m_standard_archetype = {
		.shader_asset = renderer::ShaderLibrary::Ptr->LoadShader({
			.source_file_path = "art://shaders/standard.slang",
			.entry_points = {
				{.name = "VertexMain",		.stage = rhi::ShaderStage::VS },
				{.name = "FragmentMain",	.stage = rhi::ShaderStage::PS }
			}
		})
	};	
	m_standard_archetype.BuildReflectionCache();

	PHX_INFO("Creating test PSO");
	m_standard_pso = CreateTestPso(*m_standard_archetype.shader_asset);

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

	const size_t material_stride = 256;
	const size_t max_materials = 10;
	m_material_shadow_data = MemoryBuffer(material_stride * max_materials);

#if false
	const char* test_prefab_path = "art://prefabs/box_vertex_colour/BoxVertexColors.gltf";
#else
	const char* test_prefab_path = "art://prefabs/cube/Cube.gltf";
#endif

	PHX_INFO("Loading Test Resources '{0}'", test_prefab_path);
	m_spawn_requests.push_back(ResourceManager::Load<PrefabResource>(test_prefab_path));
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

	if (m_standard_pso.IsValid())
		phx::rhi::DeletePipeline(m_standard_pso);

}

void PhxRuntime::OnPreRender(IAllocator* frame_allocator)
{
	m_render_thread_mtl_shadow_size = m_material_shadow_data.Size();
	m_render_thread_mtl_shadow_data = (uint8_t*)frame_allocator->Allocate(m_render_thread_mtl_shadow_size, 16u);
	memcpy(m_render_thread_mtl_shadow_data, m_material_shadow_data.Data(), m_material_shadow_data.Size());

	auto group = m_world.GetRegistry().group<StaticMeshComponent>(entt::get<WorldTransformComponent>);
	const size_t max_num_packets = group.size();
	m_num_render_packets = 0;

	if (max_num_packets == 0)
		return;

	static constexpr size_t MAX_NUM_PER_MESH_DRAWS = 6;
	m_render_packets = AllocateArray<RenderPacket>(frame_allocator, max_num_packets * MAX_NUM_PER_MESH_DRAWS).data();

	for (auto entity : group)
	{
		const auto& mesh_component				= group.get<StaticMeshComponent>(entity);
		const auto& world_transform_component	= group.get<WorldTransformComponent>(entity);

		auto mesh_resource = ResourceManager::Get<renderer::MeshResource>(mesh_component.mesh);
		if (mesh_resource->state != ResourceState::Loaded)
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

		for (size_t i = 0; i < m_requires_patching.size();)
		{
			auto& mat = m_requires_patching[i];
			std::byte* shadow_data_ptr = m_material_shadow_data.Data() + (mat->shadow_data_index * 256);
			bool requires_patching = false;
			for (auto& var : mat->variables)
			{
				if (var.value.type != renderer::MaterialPropertyType::Texture)
					continue;

				if (var.value.texture->state != ResourceState::Loaded)
					requires_patching = true;

				m_standard_archetype.SetTexture(shadow_data_ptr, var.name, *static_cast<renderer::TextureResource*>(var.value.texture.Get()));
			}

			if (!requires_patching)
			{
				std::swap(mat, m_requires_patching.back());
				m_requires_patching.pop_back();
			}
			else
			{
				i++;
			}
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
			packet.pso = m_standard_pso;
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
	const float rotation_speed = 0.05f; 
	const float3 steady_axis(0.0f, 1.0f, 0.0f);
	for (auto entity : view)
	{
		auto& transform = view.get<TransformComponent>(entity);

		quaternion delta_rot = quaternion::rotation_axis(steady_axis, rotation_speed * delta_time);
		transform.rotation = hlslpp::normalize(hlslpp::mul(transform.rotation, delta_rot));
		transform.dirty = true;
	}

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
		world_transform.world_matrix = hlslpp::mul(hlslpp::mul(scale_matrix, rotation_matrix), translation_matrix);
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

	Renderer_RecordResourceTransitions(command_buffer);

	struct FrameData
	{
		hlslpp::float4x4 view_proj;
		hlslpp::float3 camera_pos;
		float time;
	};

	uint32_t w, h;
	GetDefaultWindowSize(w, h);

	rhi::TypedAllocation<FrameData> frame_data = rhi::AllocTyped<FrameData>();

	// -- TEST camera logic --
	{
		static const hlslpp::float3 cam_pos = hlslpp::float3(0.0f, 2.0f, -4.0f);
		static const hlslpp::float3 cam_target = hlslpp::float3(0.0f, 0.0f, 0.0f);
		static const hlslpp::float3 cam_up = hlslpp::float3(0.0f, 1.0f, 0.0f);

		const hlslpp::float4x4 view = hlslpp::float4x4::look_at(cam_pos, cam_target, cam_up);

		// 1. Setup Window & Camera Data
		const float aspect_ratio = static_cast<float>(w) / static_cast<float>(h);
		const float fov_radians = 1.047f; // ~60 degrees
		const float near_z = 0.1f;
		const float far_z = 1000.0f;

		hlslpp::frustum f = hlslpp::frustum::field_of_view_y(
			fov_radians,
			aspect_ratio,
			near_z,
			far_z
		);

		const hlslpp::projection proj(f, hlslpp::zclip::zero);
		const hlslpp::float4x4 proj_matrix = hlslpp::float4x4::perspective(proj);

		frame_data->view_proj	= hlslpp::mul(view, proj_matrix);
		frame_data->camera_pos  = cam_pos;
		frame_data->time		= phx::SystemTime::GetCurrentTick();
	}

	rhi::TypedAllocation<hlslpp::float4x4> instance_data = rhi::AllocTyped<hlslpp::float4x4>(m_num_render_packets);

	rhi::DynamicAllocation material_data = rhi::AllocDynamic(m_render_thread_mtl_shadow_size);
	std::memcpy(material_data.ptr, m_render_thread_mtl_shadow_data, m_render_thread_mtl_shadow_size);

	uint32_t current_image_index = rhi::GetSwapchainImageIndex(m_swapchain);
	rhi::BeginRendering(
		command_buffer,
		m_swapchain,
		{ .Colour = rhi::Color(0.0f, 0.0f, 0.0f, 1.0f)},
		m_depth_textures[current_image_index],
		{ .DepthStencil = {.Depth = 1.0f, .Stencil = 0 } });


	struct PushConstants
	{
		uint64_t frame_data_device_addr;
		uint64_t instance_data_device_addr;
		uint64_t material_data_device_addr;
		uint64_t vertex_buffer_device_addr;

		uint32_t material_index;
		uint32_t instance_index;

		// -- Padding of 128 ---
		uint8_t _padding[88];
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

		std::memcpy(&instance_data[i_render_packet], &render_packet.push_constants.world_matrix, sizeof(hlslpp::float4x4));
		PushConstants push = {
			.frame_data_device_addr = frame_data.device_address,
			.instance_data_device_addr = instance_data.device_address,
			.material_data_device_addr = material_data.device_address,
			.vertex_buffer_device_addr = render_packet.push_constants.vertex_buffer_address,
			.material_index = 0,
			.instance_index = 0,
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

		PrefabResourcePtr& prefab_ptr = m_spawn_requests[i];
		if (prefab_ptr->state != ResourceState::Loaded)
			continue;

		PHX_INFO("Spawning prefab into world.");
		for (const auto& node : prefab_ptr->nodes)
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
					auto& storage_mesh_component = entity.AddComponent<StaticMeshStorageComponent>();

					storage_mesh_component.mesh = mesh_node_data.mesh;
					static_mesh_component.mesh = mesh_node_data.mesh.GetHandle();

					static_mesh_component.num_materials = std::min((uint8_t)mesh_node_data.materials.size(), (uint8_t)8);

					size_t current_index = 0;
					for (size_t i = 0; i < (size_t)static_mesh_component.num_materials; ++i)
					{
						std::byte* shadow_data_ptr = m_material_shadow_data.Data() + (current_index * 256);						
						MaterialResourcePtr material_ptr = mesh_node_data.materials[i];
						material_ptr->shadow_data_index = (uint32_t)current_index;

						bool requires_patching = false;
						for (auto& var : material_ptr->variables)
						{
							switch (var.value.type)
							{
							case renderer::MaterialPropertyType::Float:
								m_standard_archetype.SetFloat(shadow_data_ptr, var.name, var.value.float_val);
								break;
							case renderer::MaterialPropertyType::Int:
								m_standard_archetype.SetInt(shadow_data_ptr, var.name, var.value.int_val);
								break;
							case renderer::MaterialPropertyType::Bool:
								m_standard_archetype.SetBool(shadow_data_ptr, var.name, var.value.bool_val);
								break;
							case renderer::MaterialPropertyType::Float2:
								m_standard_archetype.SetFloat2(shadow_data_ptr, var.name, var.value.float2_val);
								break;
							case renderer::MaterialPropertyType::Float3:
								m_standard_archetype.SetFloat3(shadow_data_ptr, var.name, var.value.float3_val);
								break;
							case renderer::MaterialPropertyType::Float4:
								m_standard_archetype.SetFloat4(shadow_data_ptr, var.name, var.value.float4_val);
								break;
							case renderer::MaterialPropertyType::Texture:
								if (var.value.texture->state != ResourceState::Loaded)
								{
									requires_patching = true;
								}

								m_standard_archetype.SetTexture(shadow_data_ptr, var.name, *static_cast<renderer::TextureResource*>(var.value.texture.Get()));
								break;
							default:
								break;
							}
						}

						if (requires_patching)
						{
							m_requires_patching.push_back(material_ptr);
						}
					}
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

void PhxRuntime::Renderer_RecordResourceTransitions(rhi::CmdHandle command_buffer)
{
	constexpr double k_max_time_ms = 0.5;
	constexpr size_t k_max_count = 64;

	static std::vector<GenericHandle> s_active_work;
	static size_t s_active_cursor = 0;

	if (s_active_cursor >= s_active_work.size())
	{
		s_active_work.clear();
		s_active_cursor = 0;
	}

	static std::vector<GenericHandle> incoming;
	incoming.clear();
	ResourceManager::PopPendingGpuTransitions(incoming);

	if (!incoming.empty())
	{
		s_active_work.insert(s_active_work.end(), incoming.begin(), incoming.end());
	}

	if (s_active_cursor >= s_active_work.size())
		return;

	CpuTimer timer;
	size_t processed_count = 0;
	StaticArray<rhi::GpuBarrier, 16> barriers;
	size_t barrier_count = 0;
	for (size_t i = s_active_cursor; i < s_active_work.size(); ++i)
	{
		GenericHandle h = s_active_work[i];
		bool completed = ResourceManager::CollectPendingGpuTransitions(h, barriers, barrier_count);

		if (!completed)
		{
			rhi::InsertBarriers(command_buffer, barriers);
			barrier_count = 0;

			ResourceManager::CollectPendingGpuTransitions(h, barriers, barrier_count);
		}

		if (barrier_count == 16)
		{
			rhi::InsertBarriers(command_buffer, barriers);
			barrier_count = 0;
		}

		ResourceManager::SetState(h, ResourceState::Loaded);
		processed_count++;

		if (processed_count >= k_max_count) 
			break;

		if (processed_count % 10 == 0)
		{
			if (timer.Elapsed().GetMilliseconds() >= 2)
			{
				PHX_WARN(
					"GPU Transisions excited allocated frame time of {0}MS. Deferring spawns until next frame",
					k_max_time_ms);
				break;
			}
		}
	}

	if (barrier_count > 0)
	{
		rhi::InsertBarriers(command_buffer, { barriers.data, barrier_count });
	}
	
	s_active_cursor += processed_count;
}
#endif