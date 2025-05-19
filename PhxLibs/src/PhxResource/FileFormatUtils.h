#pragma once

#include <PhxCore/Base.h>
namespace phx
{

	namespace FileFormat
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
				static_cast<uint32_t>(d) << 24 |
				static_cast<uint32_t>(c) << 16 |
				static_cast<uint32_t>(b) << 8 |
				static_cast<uint32_t>(a);
		}

		enum class CompressionType : uint16_t
		{
			None = 0,
			GDeflate = 1,
		};

		template <typename T, typename TOffset = std::uint32_t>
		struct RelativePtr
		{
			TOffset Offset;

			void Set(void* ptr) { Offset = static_cast<size_t>(ptrdiff_t(ptr) - ptrdiff_t(this)); }

			T* Get() { return (T*)(((char*)this) + Offset); }
			const T* Get() const { return (const T*)(((char*)this) + Offset); }

			operator T* () { return Get(); }
			T* operator->() { return Get(); }
			T const* operator->() const { return Get(); }
		};
		
		//
		// A pointer/offset.  On disk, this is an offset relative to the containing
		// region (or the start of the file if this Ptr is stored in the header.)
		// After the data has been loaded, the offsets are fixed up and converted
		// into typed pointers.
		//
		template<typename T, typename TOffset = std::uint32_t>
		union Ptr
		{
			TOffset Offset;
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

		struct StringEntry
		{
			uint32_t Hash; // Hash of filename for lookup
			FileFormat::RelativePtr<char> Value;
		};

	}
}