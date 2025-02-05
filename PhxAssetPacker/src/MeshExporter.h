#pragma once

#include "MeshData.h"
namespace phx
{
	class MeshExporter
	{
	public:
		static void Export(phx::MeshData& data)
		{
			MeshExporter exporter;
			exporter.Export();
		}

	private:
		MeshExporter();
		void Export();
	};
}

