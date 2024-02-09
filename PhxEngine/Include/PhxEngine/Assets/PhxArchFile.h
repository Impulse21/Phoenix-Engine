#pragma once

#include <string>
namespace PhxEngine::Assets
{
	class PhxPktFile
	{
	public:
		PhxPktFile(std::string const& filename) 
			: m_filename(filename)
		{};

		virtual void StartMetadataLoad() = 0;

	protected:
		std::string m_filename;
	};

}

