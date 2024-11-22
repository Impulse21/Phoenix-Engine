#pragma once

namespace phx
{
	constexpr uint32_t MakeMagicNum(char a, char b, char c, char d)
	{
		return
			static_cast<uint32_t>(d) << 24	| 
			static_cast<uint32_t>(c) << 16	|
			static_cast<uint32_t>(b) << 8	|
			static_cast<uint32_t>(a);
	}

	enum class CompressionType : uint16_t
	{
		None = 0,
		GDeflate = 1,
	};

	template <typename T>
	struct RelativePtr
	{
		size_t offset;

		T* Get() { return (T*)(((char*)this) + offset); }
		const T* Get() const { return (const T*)(((char*)this) + offset); }
		void Set(void* ptr) { offset = static_cast<size_t>(ptrdiff_t(ptr) - ptrdiff_t(this)); }
	};
	
	//
	// An array - stored as a Ptr, with overloaded array access operators.
	//
	template<typename T>
	struct Array
	{
		RelativePtr<T> Data;

		T& operator[] (size_t index)
		{
			return Data.Get()[index];
		}

		T const& operator[] (size_t index) const
		{
			return Data.Get()[index];
		}
	};
	
	struct Chunk
	{
		CompressionType Compression = CompressionType::None;
		uint64_t Offset = 0u;
		uint32_t CompressedSize = 0u;
		uint32_t UncompressedSize = 0u;
	};

	struct DependencyEntry
	{
		char PathOrHash[256]; // File path or unique hash
	};

	struct DependencyList
	{
		uint64_t DependencyCount;
		Array<DependencyEntry> Dependencies;
	};

	namespace MeshFileFormat
	{
		constexpr uint16_t cCurrentVersion = 1u;
		struct Header
		{

			uint32_t MagicNumber = MakeMagicNum('P', 'M', 'S', 'H');
			uint16_t Version = cCurrentVersion;

			Chunk Dependencies;
			Chunk Metadata;
			Chunk CpuData;
			Chunk GpuData;

		};
	}
}

#if false

    std::cout << "Hello World!\n";

    BinaryBuilder fileOutputBuilder = {};
    BinaryBuilder::OffsetHandle headerOffset = fileOutputBuilder.Reserve<MeshFileFormat::Header>(4_KiB);

	BinaryBuilder dependencyBuilder = {};
	{
		std::vector<DependencyEntry> deps;
        DependencyEntry& entry =  deps.emplace_back();
        strcpy_s(entry.PathOrHash, sizeof(entry.PathOrHash), "materialA.mat");

		BinaryBuilder::OffsetHandle depListOffset = dependencyBuilder.Reserve<DependencyList>();
		BinaryBuilder::OffsetHandle depEntriesOffset = dependencyBuilder.Reserve(deps.size() * sizeof(DependencyEntry));

		dependencyBuilder.Commit();
		DependencyList* list = dependencyBuilder.Place<DependencyList>(depListOffset);
        list->DependencyCount = deps.size();

		DependencyEntry* entries = dependencyBuilder.Place<DependencyEntry>(depEntriesOffset);
		list->Dependencies.Data.Set(entries);

        for (int i = 0 ; i < list->DependencyCount; i++)
        {
	        entries[i] = deps[i];
        }
        
    }

    BinaryBuilder::OffsetHandle dependcyListOffset = fileOutputBuilder.Reserve(dependencyBuilder.GetMemory().Size(), 4_KiB);

	fileOutputBuilder.Commit();
    
    MeshFileFormat::Header* header = fileOutputBuilder.Place<MeshFileFormat::Header>(headerOffset);
    *header = {};
    header->Dependencies.CompressedSize = header->Dependencies.UncompressedSize = dependencyBuilder.GetMemory().Size();
    header->Dependencies.Offset = dependcyListOffset;

    void* chunkPtr = fileOutputBuilder.Place(dependcyListOffset);
    std::memcpy(chunkPtr, dependencyBuilder.GetMemory().begin(), dependencyBuilder.GetMemory().Size());
    auto memory = fileOutputBuilder.GetMemory();
    
    auto fs = phx::FileSystemFactory::CreateNativeFileSystem();
    fs->WriteFile("TestMesh.phxmesh", Span((char*)memory.begin(), memory.Size()));


    auto blob = fs->ReadFile("TestMesh.phxmesh");

    const MeshFileFormat::Header* newHeader = reinterpret_cast<const MeshFileFormat::Header*>(blob->Data());
	const DependencyList* dependicyList = reinterpret_cast<const DependencyList*>((char*)blob->Data() + newHeader->Dependencies.Offset);

    for (int i = 0; i < dependicyList->DependencyCount; i++)
    {
	    const DependencyEntry& entry = dependicyList->Dependencies[i];
	    std::cout << entry.PathOrHash << std::endl;
    }
#endif