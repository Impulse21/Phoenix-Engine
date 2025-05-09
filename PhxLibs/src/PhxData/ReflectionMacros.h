#pragma once

#define PHX_REFLECT(ClassName) \
	inline static struct __AutoRegister_##ClassName { \
		__AutoRegister_##ClassName() { Reflect(); } \
		static void Reflect() { \
			using ThisType = ClassName; \
			using BaseType = void; \
			data::TypeDescriptor type = data::TypeDescriptor::Create( #ClassName ); \
			if constexpr (!std::is_same_v<BaseType, void>) \
				type.Base.Id = data::GetId<BaseType>();

#define PHX_REFLECT_DERIVED(ClassName, BaseClass) \
	inline static struct __AutoRegister_##ClassName { \
		__AutoRegister_##ClassName() { Reflect(); } \
		static void Reflect() { \
			using ThisType = ClassName; \
			using BaseType = BaseClass; \
			data::TypeDescriptor type = data::TypeDescriptor::Create( #ClassName ); \
			type.Base.Id = data::GetId<BaseType>();

#define PHX_REFLECT_FIELD(FieldName) \
	type.AddField(data::FieldDescriptor::Create<ThisType, decltype(ThisType::FieldName), &ThisType::FieldName>(#FieldName));

#define PHX_REFLECT_END() \
			data::TypeRegistry::AddType(std::move(type)); \
		} \
	} __autoRegisterInstance_##ClassName;