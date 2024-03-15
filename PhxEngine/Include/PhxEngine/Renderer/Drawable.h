#pragma once

#include <PhxEngine/Resource/Resource.h>
#include <PhxEngine/RHI/PhxRHI.h>

namespace PhxEngine
{
	class Drawable : public Resource
	{
	public:
		Drawable() = default;
		// Construct the Drawable

		struct Geometry
		{
			// material Data;
			// Vertex Offset;
			// Index Offset;
		};

	private:
		RHI::BufferHandle m_gpuBufferData;

		// TODO: Views?
	};

}
