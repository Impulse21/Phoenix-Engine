#pragma once

#include "MeshData.h"
#include <iostream>

namespace phx
{
	class MeshFileBinaryWriter
	{
	public:
		static std::unique_ptr<uint8_t[]> Write(phx::MeshData& data)
		{
			MeshFileBinaryWriter writer(data);
			return writer.Write();
		}

	private:
		MeshFileBinaryWriter(phx::MeshData& meshData)
			: m_meshData(meshData)
		{
		}

		std::unique_ptr<uint8_t[]> Write();

	private:
		phx::MeshData& m_meshData;
	};
}

