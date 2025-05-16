
#include <PhxCore/Base.h>
#include <PhxCore/VFS.h>
#include <PhxEngine/EntryPoint.h>

#include "MeshResourceCompiler.h"
#include <fast_obj/fast_obj.h>
#include <meshoptimizer/meshoptimizer.h>

#include "Generated/GlobalVariables.h"

namespace
{
	void CompileObjAndMaterials(const char* filename, const char*)
	{
		phxed::MeshData meshSrc = {};
		if (!ParseObj(filename, meshSrc))
			return;

		// Mesh Optimizer
		const size_t totalIndices = meshSrc.Vertex_Positions.size();
		std::array<meshopt_Stream, 3> vertexStream =
		{
			meshopt_Stream{
				.data = meshSrc.Vertex_Positions.data(),
				.size = meshSrc.Vertex_Positions.size(),
				.stride = sizeof(meshSrc.Vertex_Positions[0])
			},
			meshopt_Stream{
				.data = meshSrc.Vertex_Normals.data(),
				.size = meshSrc.Vertex_Normals.size(),
				.stride = sizeof(meshSrc.Vertex_Normals[0])
			},
			meshopt_Stream{
				.data = meshSrc.Vertex_Uvset_0.data(),
				.size = meshSrc.Vertex_Uvset_0.size(),
				.stride = sizeof(meshSrc.Vertex_Uvset_0[0])
			},
		};

		std::vector<uint32_t> remap(totalIndices);
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

		processedMesh.Vertex_Positions.resize(totalVertices);
		meshopt_remapVertexBuffer(
			processedMesh.Vertex_Positions.data(),
			meshSrc.Vertex_Positions.data(),
			totalIndices,
			sizeof(processedMesh.Vertex_Positions[0]),
			&remap[0]);
	
		processedMesh.Vertex_Normals.resize(totalVertices);
		meshopt_remapVertexBuffer(
			processedMesh.Vertex_Normals.data(),
			meshSrc.Vertex_Normals.data(),
			totalIndices,
			sizeof(processedMesh.Vertex_Normals[0]),
			&remap[0]);

		processedMesh.Vertex_Uvset_0.resize(totalVertices);
		meshopt_remapVertexBuffer(
			processedMesh.Vertex_Uvset_0.data(),
			meshSrc.Vertex_Uvset_0.data(),
			totalIndices,
			sizeof(processedMesh.Vertex_Uvset_0[0]),
			&remap[0]);


	}

	bool ParseObj(const char* filename, phxed::MeshData& meshData)
	{
		fastObjMesh* obj = fast_obj_read(filename);
		if (!obj)
		{
			PHX_ERROR("Failed to Load. \n\tError {0}\n\tWarn {1}");
			return false;
		}

		size_t totalIndices = 0;

		for (uint32_t i = 0; i < obj->face_count; ++i)
			totalIndices += 3 * (obj->face_vertices[i] - 2);

		meshData.Vertex_Positions.reserve(totalIndices);
		meshData.Vertex_Normals.reserve(totalIndices);
		meshData.Vertex_Tangents.reserve(totalIndices);
		meshData.Vertex_Uvset_0.reserve(totalIndices);
		meshData.Vertex_Uvset_1.reserve(totalIndices);

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
					meshData.Vertex_Positions[vertexOffset + 0] = meshData.Vertex_Positions[vertexOffset - 3];
					meshData.Vertex_Normals[vertexOffset + 0] = meshData.Vertex_Normals[vertexOffset - 3];
					meshData.Vertex_Uvset_0[vertexOffset + 0] = meshData.Vertex_Uvset_0[vertexOffset - 3];

					meshData.Vertex_Positions[vertexOffset + 1] = meshData.Vertex_Positions[vertexOffset - 1];
					meshData.Vertex_Normals[vertexOffset + 1] = meshData.Vertex_Normals[vertexOffset - 1];
					meshData.Vertex_Uvset_0[vertexOffset + 1] = meshData.Vertex_Uvset_0[vertexOffset - 1];

					vertexOffset += 2;
				}

				meshData.Vertex_Positions[vertexOffset] =
				{
					obj->positions[gi.p * 3 + 0],
					obj->positions[gi.p * 3 + 1],
					obj->positions[gi.p * 3 + 2],
				};

				meshData.Vertex_Normals[vertexOffset] =
				{
					obj->normals[gi.n * 3 + 0],
					obj->normals[gi.n * 3 + 1],
					obj->normals[gi.n * 3 + 2],
				};

				meshData.Vertex_Uvset_0[vertexOffset] =
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
	// phx::FileSystem::Mount("native://", "");
	// phx::FileSystem::Mount("res://", phx::GlobalPaths::AssetsDirectory);
	// Import Resource
	const char* filename = "C:/Users/dipao/OneDrive/Documents/Art/SM_Chest_01.obj";
	CompileObjAndMaterials(filename, "res://modulardungeoncollection");

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