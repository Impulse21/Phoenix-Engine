#pragma once

#include "phxResource.h"
#include "Core/phxRefCountPtr.h"
#include "RHI/PhxRHI.h"


namespace phx
{
	class Mesh : public RefCounter<IResource>
	{
	public:
		Mesh();

	private:
		rhi::BufferHandle m_vertexData;
		rhi::BufferHandle m_indexData;
		
		std::vector<rhi::DrawArgs> m_drawArgs;
	};

	using MeshRef = RefCountPtr<Mesh>;

}

