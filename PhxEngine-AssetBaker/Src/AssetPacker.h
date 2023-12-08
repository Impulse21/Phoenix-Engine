#pragma once

#include <PhxEngine/Assets/AssetFile.h>
#include "ModelInfoBuilder.h"

class AssetPacker
{
public:
	virtual ~AssetPacker() = default;

	static void Pack(const char* filename, MeshInfo& meshInfo);

protected:
	AssetPacker(std::ostream& out)
		: m_out(out) {}
protected:

	void Patch(std::ostream& s, std::streampos pos, void const* data, size_t size)
	{
		auto oldPos = s.tellp();
		s.seekp(pos);
		s.write(reinterpret_cast<char const*>(data), size);
		s.seekp(oldPos);
	}

	template<typename T>
	void Patch(std::ostream& s, std::streampos pos, T const& value)
	{
		Patch(s, pos, &value, sizeof(value));
	}

	template<typename T>
	void Patch(std::ostream& s, std::streampos pos, std::vector<T> const& values)
	{
		Patch(s, pos, values.data(), values.size() * sizeof(T));
	}

	template<typename T>
	class Fixup
	{
		std::streampos m_pos;

	public:
		Fixup() = default;

		Fixup(std::streampos fixupPos)
			: m_pos(fixupPos)
		{
		}

		void Set(std::ostream& stream, T const& value) const
		{
			Patch(stream, m_pos, value);
		}

		// Overload that only works when T is a marc::Ptr<> (SFINAE)
		void Set(std::ostream& stream, std::streampos value) const
		{
			T t;
			t.OffsetFromRegionStart = static_cast<uint64_t>(value);
			Set(stream, t);
		}
	};

	template<typename T, typename FIXUP>
	Fixup<FIXUP> MakeFixup(std::streampos startPos, T const* src, FIXUP const* fixup)
	{
		auto byteSrc = reinterpret_cast<uint8_t const*>(src);
		auto byteField = reinterpret_cast<uint8_t const*>(fixup);
		std::ptrdiff_t offset = byteField - byteSrc;

		if ((offset + sizeof(*fixup)) > sizeof(*src))
		{
			throw std::runtime_error("Fixup outside of src structure");
		}

		std::streampos fixupPos = startPos + offset;

		return Fixup<FIXUP>(fixupPos);
	}


	template<typename T, typename... FIXUPS>
	std::tuple<Fixup<FIXUPS>...> WriteStruct(std::ostream& out, T const* src, FIXUPS const*... fixups)
	{
		auto startPos = out.tellp();
		out.write(reinterpret_cast<char const*>(src), sizeof(*src));

		return std::make_tuple(MakeFixup(startPos, src, fixups)...);
	}

protected:
	std::ostream& m_out;
};