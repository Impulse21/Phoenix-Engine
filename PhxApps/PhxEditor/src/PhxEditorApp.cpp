
#include <PhxCore/Base.h>
#include <PhxCore/SystemTime.h>
#include <PhxCore/Profiler.h>

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

	void SetWindowHandle(void* handle) override { m_windowHandle = handle; }
	void* GetWindowHandle() const override { return m_windowHandle; }

private:
	void TEST_RotateEntity(float deltaTime, phx::TransformComponent& comp);

private:
	inline static PhxEditor* ms_instance = nullptr;
	const phx::ApplicationDescriptor m_desc;
	phx::World m_world;
	void* m_windowHandle;
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

	phx::RefCountPtr<phx::SceneBlueprint> scene_blueprint = asset_manager->Get<phx::SceneBlueprint>(kDefault3DModel);

	// phx::RefCountPtr<phx::SceneBlueprint> sceneBlueptin = resourceSystem->GetTyped<phx::SceneBlueprint>(kDefault3DModel);

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
	//phx::gfx::IRenderSystem::Ptr->PreRender(m_world);
}

void PhxEditor::OnUpdate_Threaded(float delta_time)
{
	PHX_PROFILE;

#if false
	auto view = m_world.GetAllEntitiesWith<phx::TransformComponent, phx::MeshComponent>();

	view.each([&](entt::entity, phx::TransformComponent& transformComp, phx::MeshComponent&) {
			TEST_RotateEntity(deltaTime, transformComp);
		});
#endif

	phx::data::IStreamingManager::Ptr->Tick(delta_time);
	// Rotate cube in a random direction
}

void PhxEditor::OnRender_Threaded()
{
	PHX_PROFILE;
	// TODO: Make use of the render graph
	//phx::gfx::IRenderSystem::Ptr->Render(phx::gfx::RenderPass::Forward);
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
