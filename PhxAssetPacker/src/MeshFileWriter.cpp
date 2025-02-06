#include "MeshFileWriter.h"

#include "PhxCore/IO/ChunkFile.h"
#include "PhxCore/BinaryBuilder.h"
#include "PhxRenderer/MeshResource.h"

using namespace phx;

std::unique_ptr<uint8_t[]> phx::MeshFileBinaryWriter::Write()
{
	BinaryBuilder binBuilder;

	const OffsetHandle handleHeader = binBuilder.Reserve<ChunkFileFormat::Header>();

	// Reserve Chuncks
	const OffsetHandle handleGpuDataHeader = binBuilder.Reserve<ChunkFileFormat::ChunkHeader>();
	const OffsetHandle handleCpuDataHeader = binBuilder.Reserve<ChunkFileFormat::ChunkHeader>();

	// Reserve Cpu Data
	const OffsetHandle handleCpuMetadataHeader = binBuilder.Reserve<renderer::MeshCpuMetadata>();
	const OffsetHandle handleGpuDataHeader = binBuilder.Reserve();
	return std::unique_ptr<uint8_t[]>();
}
