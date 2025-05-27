#pragma once

#include "Any.h"
namespace phx
{
	class IFileSystem;
}
namespace phx::data
{
	void Save(IFileSystem* fs, const char* filename, data::AnyPtr object);

	template<class T>
	void Save(IFileSystem* fs, const char* filename, T const& object)
	{
		Save(
			fs,
			filename,
			AnyPtr{
				.Value = &object,
				.TypeId = GetId<T>() });
	}

	void Load(IFileSystem* fs, const char* filename, data::AnyPtr object);

	template<class T>
	void Load(IFileSystem* fs, const char* filename, T& object)
	{
		Load(
			fs,
			filename,
			AnyPtr{
				.Value = &object,
				.TypeId = GetId<T>() });
	}
}
