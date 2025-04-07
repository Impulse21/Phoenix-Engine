#include "PhxData_pch.h"
#include "Reflection.h"
#include "WorldComponents.def"


using namespace phx;
using namespace phx::rft;

template<>
struct TypeInfo<TransformComponent>
{
	static constexpr FieldInfo Fields[] = {
		{ "Scale", "DirectX::XMFLOAT3", "DirectX::XMFLOAT3"_hash, "Local Scale", offsetof(TransformComponent, LocalScale), std::initializer_list<ExtraInfo>{} },
		{ "Rotation", "DirectX::XMFLOAT4", "DirectX::XMFLOAT4"_hash, "Local Rotation", offsetof(TransformComponent, LocalRotation), std::initializer_list<ExtraInfo>{} },
		{ "Translation", "DirectX::XMFLOAT3", "DirectX::XMFLOAT3"_hash, "Local Translation", offsetof(TransformComponent, LocalTranslation), std::initializer_list<ExtraInfo>{} },
	};

	static constexpr phx::Span<const FieldInfo> GetFields() { return Fields; }
	static constexpr const char* GetTypeName() { return "TransformComponent"; }
	static constexpr StringHash GetTypeNameHash() { return "TransformComponent"_hash; }
};

