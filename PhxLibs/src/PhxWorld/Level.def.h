#pragma once

#include <PhxData/Reflection.h>
#include <PhxData/DataContainers.h>

#include <DirectXMath.h>

namespace cereal {

	template <class Archive>
	void save(Archive& archive, DirectX::XMFLOAT3& v)
	{
		archive(v.x, v.y, v.z);
	}

	template <class Archive>
	void load(Archive& archive, DirectX::XMFLOAT4& v)
	{
		archive(v.x, v.y, v.z, v.w);
	}

} // namespace cereal

namespace phx
{
	struct LevelObject;

	struct Level
	{
		std::string Name;
		std::unique_ptr<LevelObject> RootObject;

		template <class Archive>
		void save(Archive& ar)
		{
			ar(Name, RootObject);
		}

		template <class Archive>
		void load(Archive& ar)
		{
			ar(Name, RootObject);
		}
#if false
		PHX_REFLECT(Level)
			PHX_REFLECT_FIELD(Name)
			PHX_REFLECT_FIELD(RootObject)
		PHX_REFLECT_END()
#endif
	};

	struct LevelObject
	{
		std::string Name;
		DirectX::XMFLOAT3 Scale = { 1.0f, 1.0f, 1.0f };
		DirectX::XMFLOAT4 Rotation = { 0.0f, 0.0f, 0.0f, 1.0f };
		DirectX::XMFLOAT3 Translation = { 0.0f, 0.0f, 0.0f };

		std::vector<std::unique_ptr<LevelObject>> Children;

		template <class Archive>
		void save(Archive& ar)
		{
			ar(Name, Scale, Rotation, Translation, Children);
		}

		template <class Archive>
		void load(Archive& ar)
		{
			ar(Name, Scale, Rotation, Translation, Children);
		}
#if false
		PHX_REFLECT(LevelObject)
			PHX_REFLECT_FIELD(Name)
			PHX_REFLECT_FIELD(Scale)
			PHX_REFLECT_FIELD(Rotation)
			PHX_REFLECT_FIELD(Translation)
			PHX_REFLECT_FIELD(Children)
		PHX_REFLECT_END()
#endif
	};
}