#include "MeshResourceExporter.h"


#include <PhxEngine/Core/BinaryBuilder.h>

using namespace PhxEngine;


PhxEngine::Pipeline::MeshResourceExporter::MeshResourceExporter(std::ostream& out, Compression compression, Mesh const& mesh)
	: m_out(out)
	, m_compression(compression)
	, m_mesh(mesh)
{
}

void PhxEngine::Pipeline::MeshResourceExporter::Export()
{
	// Prepare memory for export
	BinaryBuilder fileBuilder;
	size_t headerOffset = fileBuilder.Reserve<MeshFileFormat::Header>();

	BinaryBuilder gpuRegionBuilder;
	GpuRegion gpuRegion = this->CreateGpuRegion(gpuRegionBuilder);
	gpuRegion.Data = fileBuilder.Reserve(gpuRegionBuilder);

	BinaryBuilder meshMetadataBuilder;
	Region<MeshFileFormat::MeshMetadataHeader> meshMetadataRegion = this->CreateMeshMetadataRegion(meshMetadataBuilder);
	meshMetadataRegion.Data = fileBuilder.Reserve(gpuRegionBuilder);


	fileBuilder.Commit();

	MeshFileFormat::Header* header = fileBuilder.Place<MeshFileFormat::Header>(headerOffset);
	header->ID = MeshFileFormat::ID;
	header->Version = MeshFileFormat::CurrentVersion;
	header->GpuData = gpuRegion;
	header->MeshMetadataHeader = meshMetadataRegion;

	header->BoundingSphere[0] = this->m_mesh.BoundingSphere.Centre.x;
	header->BoundingSphere[1] = this->m_mesh.BoundingSphere.Centre.y;
	header->BoundingSphere[2] = this->m_mesh.BoundingSphere.Centre.z;
	header->BoundingSphere[3] = this->m_mesh.BoundingSphere.Radius;

	header->MinPos[0] = this->m_mesh.BoundingBox.Min.x;
	header->MinPos[1] = this->m_mesh.BoundingBox.Min.y;
	header->MinPos[2] = this->m_mesh.BoundingBox.Min.z;

	header->MaxPos[0] = this->m_mesh.BoundingBox.Max.x;
	header->MaxPos[1] = this->m_mesh.BoundingBox.Max.y;
	header->MaxPos[2] = this->m_mesh.BoundingBox.Max.z;

	fileBuilder.Place(gpuRegion.Data, gpuRegionBuilder);
	fileBuilder.Place(meshMetadataRegion.Data, meshMetadataBuilder);

	this->m_out.write(fileBuilder.Data(), fileBuilder.Size());
}

Region<MeshFileFormat::MeshMetadataHeader> PhxEngine::Pipeline::MeshResourceExporter::CreateMeshMetadataRegion(BinaryBuilder& regionBuilder)
{
	return Region<MeshFileFormat::MeshMetadataHeader>();
}

GpuRegion PhxEngine::Pipeline::MeshResourceExporter::CreateGpuRegion(BinaryBuilder& regionBuilder)
{
	return GpuRegion();
}
