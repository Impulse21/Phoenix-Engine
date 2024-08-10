#pragma once

#include "Core/phxUUID.h"
#include "Core/phxStringHash.h"

namespace phx
{

	struct IDComponent
	{
		UUID ID;
	};

	struct NameComponent
	{
		std::string Name;
		StringHash Hash;

		inline void operator=(const std::string& str) { this->Name = str; this->Hash = str.c_str(); }
		inline void operator=(std::string&& str) { this->Name = std::move(str); this->Hash = this->Name.c_str();}
		inline bool operator==(const std::string& str) const { return this->Name.compare(str) == 0; }

	};
}