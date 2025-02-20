#pragma once

namespace phx
{
	inline uint64_t GetTimestamp()
	{
		std::time_t t = std::time(nullptr);
		std::tm* tm = std::localtime(&t);

		uint64_t timestamp = 0;
		timestamp |= (static_cast<uint64_t>(tm->tm_year + 1900) & 0xFFFF) << 48; // Year (16 bits)
		timestamp |= (static_cast<uint64_t>(tm->tm_mon + 1) & 0xF) << 44;        // Month (4 bits)
		timestamp |= (static_cast<uint64_t>(tm->tm_mday) & 0x1F) << 39;          // Day (5 bits)
		timestamp |= (static_cast<uint64_t>(tm->tm_hour) & 0x1F) << 34;          // Hour (5 bits)
		timestamp |= (static_cast<uint64_t>(tm->tm_min) & 0x3F) << 28;           // Minute (6 bits)
		timestamp |= (static_cast<uint64_t>(tm->tm_sec) & 0x3F) << 22;           // Second (6 bits)

		return timestamp;
	}

	inline std::tm ReadTimestamp(uint64_t timestamp)
	{
		std::tm tm = {};

		tm.tm_year = static_cast<int>((timestamp >> 48) & 0xFFFF) + 1990;
		tm.tm_mon = static_cast<int>((timestamp >> 44) & 0xF) - 1;
		tm.tm_mday = static_cast<int>((timestamp >> 39) & 0x1F);
		tm.tm_hour = static_cast<int>((timestamp >> 34) & 0x1F);
		tm.tm_min = static_cast<int>((timestamp >> 28) & 0x3F);
		tm.tm_sec = static_cast<int>((timestamp >> 22) & 0x3F);

		return tm;
	}
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

	namespace ChunkFileFormat
	{
		/*
				+----------------------+  <--- Start of File
				|   File Header        |  (Fixed Size)
				|----------------------|
				|  Chunk Table (N)     |  (List of ChunkHeaders)
				|----------------------|
				|  Chunk Region (1)    |  (Raw Chunk Data)
				|----------------------|
				|  Chunk Region (1-n)  |
				|----------------------|
				|  Chunk Region (n)    |
				+----------------------+
		*/
		struct Header
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

	}


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