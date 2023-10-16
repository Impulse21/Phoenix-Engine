#pragma once

#include <PhxEngine/Assets/Assets.h>
#include <memory>

namespace PhxEngine::Assets
{
	namespace AssetStore
	{
		void Initialize();
		void Finalize();

		std::shared_ptr<IMesh> RetrieveMesh();

		std::shared_ptr<IMaterial> RetrieveMaterial();

		std::shared_ptr<ITexture> RetrieveTexture();
	}
}

