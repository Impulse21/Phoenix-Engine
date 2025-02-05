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
	
	template <typename T, typename TOffset = std::int32_t>
	struct RelativePtr
	{
		TOffset offset;

		T* Get() { return (T*)(((char*)this) + offset); }
		const T* Get() const { return (const T*)(((char*)this) + offset); }
		void Set(void* ptr) { offset = static_cast<size_t>(ptrdiff_t(ptr) - ptrdiff_t(this)); }
	};

	//
	// A pointer/offset.  On disk, this is an offset relative to the containing
	// region (or the start of the file if this Ptr is stored in the header.)
	// After the data has been loaded, the offsets are fixed up and converted
	// into typed pointers.
	//
	template<typename T>
	union Ptr
	{
		uint32_t Offset;
		T* Ptr;
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
	/*
			+----------------------+  <--- Start of File
			|   File Header        |  (Fixed Size)
			|----------------------|
			|  Chunk Table (N)     |  (List of ChunkHeaders)
			|----------------------|
			|  Chunk Data          |  (Raw Chunk Data)
			+----------------------+
	*/

	struct ChunkFileHeader
	{
		uint32_t Magic;
		uint32_t Version;
		uint64_t BuildNumber;
		uint32_t ChunkCount;
		uint32_t _Pading;
	};

	struct ChunkHeader
	{
		uint32_t ChunkID;
		CompressionType Compression;
		uint8_t _Padding;
		uint32_t Offset; // (on disk this is compressed, in memory it is uncompressed)
		uint32_t CompressedSize;
		uint32_t UncompressedSize;
	};


	class ChunkFile
	{
	public:
		ChunkFile() = default;
	};
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