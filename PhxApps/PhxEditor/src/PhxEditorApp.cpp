
#include <PhxCore/Base.h>
#include <PhxCore/VFS.h>
#include <PhxCore/SystemTime.h>
#include <PhxCore/Profiler.h>

#include <PhxWorld/WorldComponents.h>
#include <PhxWorld/Entity.h>
#include <PhxWorld/World.h>
#include <PhxWorld/WorldSerializer.h>
#include <PhxWorld/SceneBlueprint.h>

#include <PhxResource/ResourceSystem.h>

#include <PhxRenderer/RenderSystem.h>
#include <PhxRenderer/RenderLayers/MeshRenderLayer.h>

#include <PhxEngine/EntryPoint.h>

#include <Generated/GlobalVariables.h>

#include "MeshResourceCompiler.h"
#include "ResourceFileBuilder.h"

#include <fast_obj/fast_obj.h>

#include <meshoptimizer/meshoptimizer.h>
#include <random>

#include "GltfFileHandler.h"

namespace
{
#if false
	// constexpr  size_t kCacheSize = 16;
	bool ParseObj(const char* filename, phxed::MeshData& meshData)
	{
		std::filesystem::path resolvedFilePath = phx::IRootFileSystem::Ptr->ResolvePath(filename);
		std::string resolvedFilename = resolvedFilePath.generic_string();
		fastObjMesh* obj = fast_obj_read(resolvedFilename.c_str());
		if (!obj)
		{
			PHX_ERROR("Failed to Load OBJ Mesh '{0}'.", filename);
			return false;
		}

		PHX_INFO("Loaded '{0}' with fast obj", filename);
		size_t totalIndices = 0;

		for (uint32_t i = 0; i < obj->face_count; ++i)
			totalIndices += 3 * (obj->face_vertices[i] - 2);

		phxed::VertexStream& positionStream = meshData.AddVertexStream<DirectX::XMFLOAT3>(phx::renderer::VertexStream_Position, totalIndices);
		phx::SpanMutable<DirectX::XMFLOAT3> positionData = positionStream.AsSpanMutable<DirectX::XMFLOAT3>();

		phxed::VertexStream& normalsStream = meshData.AddVertexStream<DirectX::XMFLOAT3>(phx::renderer::VertexStream_Normal, totalIndices);
		phx::SpanMutable<DirectX::XMFLOAT3> normalData = normalsStream.AsSpanMutable<DirectX::XMFLOAT3>();

		phxed::VertexStream& uv0Stream = meshData.AddVertexStream<DirectX::XMFLOAT2>(phx::renderer::VertexStream_UV0, totalIndices);
		phx::SpanMutable<DirectX::XMFLOAT2> uv0Data = uv0Stream.AsSpanMutable<DirectX::XMFLOAT2>();

		size_t vertexOffset = 0;
		size_t indexOffset = 0;

		for (size_t iFace = 0; iFace < obj->face_count; ++iFace)
		{
			for (size_t iVert = 0; iVert < obj->face_vertices[iFace]; ++iVert)
			{
				fastObjIndex gi = obj->indices[indexOffset + iVert];



				// triangulate polygon on the fly; offset-3 is always the first polygon vertex
				if (iVert >= 3)
				{
					positionData[vertexOffset + 0] = positionData[vertexOffset - 3];
					normalData[vertexOffset + 0] = normalData[vertexOffset - 3];
					uv0Data[vertexOffset + 0] = uv0Data[vertexOffset - 3];

					positionData[vertexOffset + 1] = positionData[vertexOffset - 1];
					normalData[vertexOffset + 1] = normalData[vertexOffset - 1];
					uv0Data[vertexOffset + 1] = uv0Data[vertexOffset - 1];

					vertexOffset += 2;
				}

				positionData[vertexOffset] =
				{
					obj->positions[gi.p * 3 + 0],
					obj->positions[gi.p * 3 + 1],
					obj->positions[gi.p * 3 + 2],
				};

				normalData[vertexOffset] =
				{
					obj->normals[gi.n * 3 + 0],
					obj->normals[gi.n * 3 + 1],
					obj->normals[gi.n * 3 + 2],
				};

				uv0Data[vertexOffset] =
				{
					obj->texcoords[gi.t * 2 + 0],
					obj->texcoords[gi.t * 2 + 1],
				};
				vertexOffset++;
			}

			indexOffset += obj->face_vertices[iFace];
		}

		meshData.Geometry.emplace_back(phxed::MeshData::GeometryData{
				.MaterialId = phx::StringHash("Default"),
				.IndexOffset = 0,
				.IndexCount = static_cast<uint32_t>(meshData.Indices.size()),
			});
		fast_obj_destroy(obj);

		return true;
	}

	phxed::MeshData GenerateMeshIndices(phxed::MeshData const& meshSrc, std::vector<uint32_t>& outRemap)
	{
		// Mesh Optimizer
		const size_t totalIndices = meshSrc.GetVertexCount();

		const phxed::VertexStream& srcPositionStream = *meshSrc.GetVertexStream(phx::renderer::VertexStream_Position);
		const phxed::VertexStream& srcNormalStream = *meshSrc.GetVertexStream(phx::renderer::VertexStream_Normal);
		const phxed::VertexStream& srcUv0Stream  = *meshSrc.GetVertexStream(phx::renderer::VertexStream_UV0);

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


		phxed::MeshData processedMesh = {};

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

		phxed::VertexStream& uvStream  = processedMesh.AddVertexStream<DirectX::XMFLOAT2>(phx::renderer::VertexStream_UV0, totalVertices);
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

	void PrintStatistics(phxed::MeshData const&)
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

	void OptimizeMesh(phxed::MeshData& mesh, std::vector<uint32_t>& remap)
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

	void CompileObjAndMaterials(const char* filename, const char* outputPath)
	{
		phxed::MeshData mesh = {};
		if (!ParseObj(filename, mesh))
			return;

		std::vector<uint32_t> remap;
		phx::CpuTimer timer;
		mesh = GenerateMeshIndices(mesh, remap);
		phx::CpuTimeStep generateIndicesTime = timer.Elapsed();

		PHX_INFO(
			"Deintrlvd: {0} vertices, reindexed in {1} msec",
			mesh.GetVertexCount(),
			generateIndicesTime.GetMilliseconds());

		OptimizeMesh(mesh, remap);

		phxed::CompiledResource compiledMesh = {};
		phxed::MeshResourceCompiler::Compile(mesh, compiledMesh);

		std::unique_ptr<phx::IBlob> resourceFileBlob = phxed::ResourceFileBuilder::Build(&compiledMesh);

		auto fs = phx::IRootFileSystem::Ptr;
		fs->FolderCreate(outputPath);

		if (!fs->WriteFile(outputPath, resourceFileBlob.get()))
			PHX_ERROR("Failed to save file '{0}'", outputPath);
		
	}

	void GenerateTestResources(const char* worldOutput)
	{
		const char* testResourceOutput = "res://modulardungeoncollection/SM_Chest_01.phxmsh";

		// Save Resource
		phx::ThreadPool::SubmitTask([&]() {

			const char* filename = "art://SM_Chest_01.obj";
			PHX_INFO("Compiling and Exporting test resource '{0}'", filename);
			CompileObjAndMaterials(filename, testResourceOutput);
		});

		// Save World
		phx::ThreadPool::SubmitTask([&]() {
			phx::World world;

			phx::Entity camera = world.CreateEntity("main_camera");
			{
				auto& cameraComp = camera.AddComponent<phx::CameraComponent>();
				cameraComp.Active = true;

				DirectX::XMVECTOR eyePos = DirectX::XMVectorSet(0.0f, 0.0f, -2.0f, 0.0f);
				DirectX::XMVECTOR focusPoint = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
				DirectX::XMVECTOR eyeDir = DirectX::XMVectorSubtract(focusPoint, eyePos);
				eyeDir = DirectX::XMVector3Normalize(eyeDir);

				DirectX::XMStoreFloat3(
					&cameraComp.Eye,
					eyePos);

				DirectX::XMStoreFloat3(
					&cameraComp.Forward,
					eyeDir);

				cameraComp.FoV = DirectX::XMConvertToRadians(60);
			}

			phx::Entity entity = world.CreateEntity("SM_Chest_01");
			phx::MeshComponent& meshComp = entity.AddComponent<phx::MeshComponent>();
			meshComp.Mesh = testResourceOutput;

			phx::WorldSerializer::Save(phx::IRootFileSystem::Ptr, worldOutput, world);
		});

		phx::ThreadPool::Wait();
	}
#endif



static const char* kDefault3DModel = "art://Sponza/glTF/Sponza.gltf";

#define InjectDefault3DModel() \
    if (raptor::file_exists(kDefault3DModel)) {\
        argc = 2;\
        argv[1] = const_cast<char*>(kDefault3DModel);\
    }\
    else {\
       printf("Unable to find default model. Please check the README in the root folder and make sure you've run `python ./bootstrap.py` to download all the additional assets for this project.\n");\
       exit(-1);\
    }
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
		.WorkingDirectory = phx::FileSystem::GetDirectoryWithExecutable()
	};

	return phx_new_persistent(PhxEditor, desc);
}

void phx::DeleteApplication(phx::IApplication* ptr)
{
	phx_delete_persistent(ptr);
}

void PhxEditor::Startup()
{
	auto fs = phx::IRootFileSystem::Ptr;
	{
		std::shared_ptr<phx::IFileSystem> nativeFS = phx::FileSystemFactory::CreateNativeFileSystem();
		fs->Mount("native://", nativeFS);

		PHX_INFO("Filesystem is mounting {0} to {1}", "res://", phx::GlobalPaths::DefaultProjectDir);
		fs->Mount("res://", phx::GlobalPaths::DefaultProjectDir);

		PHX_INFO("Filesystem is mounting {0} to {1}", "art://", phx::GlobalPaths::ArtSrcDirectory);
		fs->Mount("art://", phx::GlobalPaths::ArtSrcDirectory);
	}

	auto* resourceSystem = phx::ResourceSystem::Ptr;
	resourceSystem->RegisterFileHanlder<phxed::GltfFileHandler>();
	phx::RefCountPtr<phx::SceneBlueprint> sceneBlueptin = resourceSystem->GetTyped<phx::SceneBlueprint>(kDefault3DModel);
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

void PhxEditor::OnUpdate_Threaded(float /*deltaTime*/)
{
	PHX_PROFILE;

#if false
	auto view = m_world.GetAllEntitiesWith<phx::TransformComponent, phx::MeshComponent>();

	view.each([&](entt::entity, phx::TransformComponent& transformComp, phx::MeshComponent&) {
			TEST_RotateEntity(deltaTime, transformComp);
		});
#endif

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
