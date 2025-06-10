#pragma once

namespace phx::data
{
	class IAssetImporter
	{
	public:
		virtual void ImportAsync() = 0;
	};
}