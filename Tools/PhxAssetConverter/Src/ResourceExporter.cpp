#include "ResourceExporter.h"

#include <PhxEngine/Resource/ResourceFormats.h>

using namespace PhxEngine;

namespace
{
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

	template<typename T, typename... FIXUPS>
	std::tuple<Fixup<FIXUPS>...> WriteStruct(std::ostream& out, T const* src, FIXUPS const*... fixups)
	{
		auto startPos = out.tellp();
		out.write(reinterpret_cast<char const*>(src), sizeof(*src));

		return std::make_tuple(MakeFixup(startPos, src, fixups)...);
	}
}

PhxEngine::Pipeline::MeshResourceExporter::MeshResourceExporter(std::ostream& out, ICompressor* compressor, Mesh const& mesh)
	: ResourceExporter(out, compressor)
	, m_mesh(mesh)
{
}

void PhxEngine::Pipeline::MeshResourceExporter::Export()
{
	MeshResource::Header header = {};
	header.Id = ResourceId<MeshResource::Header>::Value;
	header.Version = ResourceVersion<MeshResource::Header>::Value;

	auto [fixupHeader] = WriteStruct(m_out, &header, &header);

	// Construct the GPU Data
}

PhxEngine::Pipeline::TextureResourceExporter::TextureResourceExporter(std::ostream& out, ICompressor* compressor, Texture const& texture)
	: ResourceExporter(out, compressor)
	, m_texture(texture)
{
}

void PhxEngine::Pipeline::TextureResourceExporter::Export()
{
}

PhxEngine::Pipeline::MaterialResourceExporter::MaterialResourceExporter(std::ostream& out, ICompressor* compressor, Material const& mtl)
	: ResourceExporter(out, compressor)
	, m_material(mtl)
{
}

void PhxEngine::Pipeline::MaterialResourceExporter::Export()
{
}
