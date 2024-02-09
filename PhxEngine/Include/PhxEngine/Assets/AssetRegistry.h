#pragma once

#include <string>
#include <memory>

namespace PhxEngine::Assets
{
	template<class T>
	class AssetRegistry
	{
	public:

	protected:
		AssetRegistry() = default;


	private:

	};


	class MeshRegistry
	{
	public:
		// Global Variable
		inline static MeshRegistry* Ptr = nullptr;
	};
}

