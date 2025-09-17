#include "ModelExporter.h"

#include <PhxRenderer/ModelResoure.h>
#include <PhxRenderer/ModelResourceHandler.h>
#include <PhxCore/BinaryBuilder.h>

#include <PhxResource/ResourceFileFormat.h>

#include "ResourceFileBuilder.h"

using namespace phx;
using namespace phx::renderer;

void ModelExporter::Export()
{
	m_compiled_resource.name = m_model_data.name;
	m_compiled_resource.ext = ResourceExtension< renderer::ModelResourceHandler>::value;

	m_compiled_resource.metadata_chunk = MemoryBuffer::Create<ModelMetadata>();

	auto metadata_view = m_compiled_resource.metadata_chunk.GetView<ModelMetadata>();
	std::memset(metadata_view.Get(), 0, m_compiled_resource.metadata_chunk.Size());

	metadata_view->geometry_bufer_size = static_cast<uint32_t>(m_model_data.geometry_data.size());

	{
		BinaryBuilder cpu_data_builder = {};
		OffsetHandle32 cpu_data_offset = cpu_data_builder.Reserve<renderer::ModelResoure::CpuData>();

		std::vector<OffsetHandle32> mesh_offsets(m_model_data.meshes.size());
		for (size_t i = 0; i < m_model_data.meshes.size(); i++)
		{
			::Mesh& mesh = *m_model_data.meshes[i];
			const size_t size_of_draw_data = sizeof(renderer::Mesh::Draw) * mesh.num_draws - 1;
			OffsetHandle32 mesh_offset = cpu_data_builder.Reserve(sizeof(renderer::Mesh) + size_of_draw_data);
			mesh_offsets.push_back(mesh_offset);
		}

		cpu_data_builder.Commit();

		auto* cpu_data = cpu_data_builder.Place<renderer::ModelResoure::CpuData>(cpu_data_offset);
		cpu_data->bounding_sphere[0] = m_model_data.bounding_sphere.centre.x;
		cpu_data->bounding_sphere[1] = m_model_data.bounding_sphere.centre.y;
		cpu_data->bounding_sphere[2] = m_model_data.bounding_sphere.centre.z;
		cpu_data->bounding_sphere[3] = m_model_data.bounding_sphere.radius;

		cpu_data->bounding_box[0] = m_model_data.bounding_box.min.x;
		cpu_data->bounding_box[1] = m_model_data.bounding_box.min.y;
		cpu_data->bounding_box[2] = m_model_data.bounding_box.min.z;
		cpu_data->bounding_box[3] = m_model_data.bounding_box.max.x;
		cpu_data->bounding_box[4] = m_model_data.bounding_box.max.y;
		cpu_data->bounding_box[5] = m_model_data.bounding_box.max.z;

		auto* mesh_data = cpu_data_builder.Place<renderer::Mesh>(mesh_offsets.front());
		cpu_data->meshes.Set(mesh_data);
		cpu_data->num_meshes = m_model_data.meshes.size();

		const OffsetHandle32 base_offset = mesh_offsets.front();
		for (size_t i = 0; i < m_model_data.meshes.size(); i++)
		{
			renderer::Mesh* mesh = cpu_data->meshes + (mesh_offsets[i] - base_offset);

			::Mesh* src_mesh = m_model_data.meshes[i].get();
			mesh->bounds[0] = src_mesh->bounds[0];
			mesh->bounds[1] = src_mesh->bounds[1];
			mesh->bounds[2] = src_mesh->bounds[2];
			mesh->bounds[3] = src_mesh->bounds[3];

			mesh->vb_offset = src_mesh->vb_offset;
			mesh->vb_offset = src_mesh->vb_offset;
			mesh->vb_size = src_mesh->vb_size;
			mesh->ib_offset = src_mesh->ib_offset;
			mesh->ib_size = src_mesh->ib_size;
			mesh->ib_format = src_mesh->ib_format;
			mesh->mesh_cbv = src_mesh->mesh_cbv;
			mesh->material_cbv = src_mesh->material_cbv;
			mesh->pso_flags = src_mesh->pso_flags;
			mesh->pso = src_mesh->pso;
			mesh->num_joints = src_mesh->num_joints;
			mesh->start_joint = src_mesh->start_joint;
			mesh->num_draws = src_mesh->num_draws;

			for (size_t j = 0; j < mesh->num_draws; j++)
			{
				mesh->draw[j].prim_count = src_mesh->draw[j].prim_count;
				mesh->draw[j].start_index = src_mesh->draw[j].start_index;
				mesh->draw[j].base_vertex = src_mesh->draw[j].base_vertex;
			}
		}

		MemoryBuffer& cpu_chunk_data = m_compiled_resource.chunks.emplace_back<MemoryBuffer>(MemoryBuffer(cpu_data_builder.GetSize()));
		std::memcpy(cpu_chunk_data.Data(), cpu_data_builder.GetMemory().data(), cpu_data_builder.GetSize());

		MemoryBuffer& geometry_chunk_data = m_compiled_resource.chunks.emplace_back<MemoryBuffer>(MemoryBuffer(m_model_data.geometry_data.size()));
		std::memcpy(geometry_chunk_data.Data(), m_model_data.geometry_data.data(), m_model_data.geometry_data.size());

	}

	ResourceFileBuilder::Build(&m_compiled_resource);
}