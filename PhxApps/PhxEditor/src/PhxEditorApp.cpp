
#include <PhxCore/Base.h>
#include <PhxCore/VFS.h>
#include <PhxCore/SystemTime.h>

#include <PhxEngine/EntryPoint.h>

#include <Generated/GlobalVariables.h>

#include "MeshResourceCompiler.h"
#include "ResourceFileBuilder.h"

#include <fast_obj/fast_obj.h>

#include <meshoptimizer/meshoptimizer.h>

namespace
{
	// constexpr  size_t kCacheSize = 16;
	bool ParseObj(const char* filename, phxed::MeshData& meshData)
	{
		std::filesystem::path resolvedPath = phx::IRootFileSystem::Ptr->ResolvePath(filename);
		fastObjMesh* obj = fast_obj_read(resolvedPath.generic_string().c_str());
		if (!obj)
		{
			PHX_ERROR("Failed to Load. \n\tError {0}\n\tWarn {1}");
			return false;
		}

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

		fast_obj_destroy(obj);

		return true;
	}

	phxed::MeshData GenerateMeshIndices(phxed::MeshData const& meshSrc, std::vector<uint32_t>& outRemap)
	{
		// Mesh Optimizer
		const size_t totalIndices = meshSrc.Vertex_Positions.size();

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

		phxed::VertexStream& posStream = processedMesh.AddVertexStream<DirectX::XMFLOAT3>(phxed::VertexStreamType::Position, totalVertices);
		meshopt_remapVertexBuffer(
			posStream.Data.get(),
			srcPositionStream.Data.get(),
			totalIndices,
			posStream.ElementStride,
			&remap[0]);

		phxed::VertexStream& normalStream = processedMesh.AddVertexStream<DirectX::XMFLOAT3>(phxed::VertexStreamType::Normal, totalVertices);
		meshopt_remapVertexBuffer(
			normalStream.Data.get(),
			srcNormalStream.Data.get(),
			totalIndices,
			normalStream.ElementStride,
			&remap[0]);

		phxed::VertexStream& uvStream  = processedMesh.AddVertexStream<DirectX::XMFLOAT2>(phxed::VertexStreamType::Uvset_0, totalVertices);
		meshopt_remapVertexBuffer(
			uvStream.Data.get(),
			srcUv0Stream.Data.get(),
			totalIndices,
			uvStream.ElementStride,
			&remap[0]);

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
		meshopt_Stream shadowStream =
			meshopt_Stream{
				.data = mesh.Vertex_Positions.data(),
				.size = sizeof(mesh.Vertex_Positions[0]),
				.stride = sizeof(mesh.Vertex_Positions[0])
		};

		mesh.ShadowIndices.resize(totalIndices);
		meshopt_generateShadowIndexBufferMulti(mesh.ShadowIndices.data(), mesh.ShadowIndices.data(), totalIndices, totalVertices, &shadowStream, 1);

		meshopt_optimizeVertexCache(mesh.ShadowIndices.data(), mesh.ShadowIndices.data(), totalIndices, totalVertices);
		phx::CpuTimeStep shadowOptimize = timer.Elapsed();

		PHX_INFO(
			"Deintrlvd: {0} vertices, optimized in {2} msec, generated & optimized shadow indices in {3} msec",
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

		if (!phx::IRootFileSystem::Ptr->WriteFile(outputPath, resourceFileBlob.get()))
			PHX_ERROR("Failed to save file '{0}'", outputPath);
		
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
	inline static PhxEditor* ms_instance = nullptr;
	const phx::ApplicationDescriptor m_desc;
	void* m_windowHandle;
};

phx::IApplication* phx::CreateApplication()
{
	ApplicationDescriptor desc = {
		.Name = "PhxEditor",
		.WorkingDirectory = phx::VFS::GetDirectoryWithExecutable()
	};

	return new PhxEditor(desc);
}

void PhxEditor::Startup()
{
	auto& fs = phx::IRootFileSystem::Ptr;
	fs->Mount("native://", phx::FileSystemFactory::CreateNativeFileSystem());
	fs->Mount("res://", phx::GlobalPaths::AssetsDirectory);

	// Import Resource
	const char* filename = "native://C:/Users/dipao/OneDrive/Documents/Art/SM_Chest_01.obj";
	CompileObjAndMaterials(filename, "res://modulardungeoncollection/SM_Chest_01.phxmsh");

	// Compile mesh and save it to disk
}

void PhxEditor::Shutdown()
{
}

void PhxEditor::OnPreRender()
{
}

void PhxEditor::OnUpdate_Threaded(float /*deltaTime*/)
{
}

void PhxEditor::OnRender_Threaded()
{
}