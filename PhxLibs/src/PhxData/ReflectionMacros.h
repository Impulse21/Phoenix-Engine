#pragma once

#define PHX_REFLECT(ClassName) \
	inline static struct __AutoRegister_##ClassName { \
		__AutoRegister_##ClassName() { Reflect(); } \
		static void Reflect() { \
			using ThisType = ClassName; \
			using BaseType = void; \
			data::TypeDescriptor type{ #ClassName }; \
			if constexpr (!std::is_same_v<BaseType, void>) \
				type.Base.Id = data::GetId<BaseType>();

#define PHX_REFLECT_DERIVED(ClassName, BaseClass) \
	inline static struct __AutoRegister_##ClassName { \
		__AutoRegister_##ClassName() { Reflect(); } \
		static void Reflect() { \
			using ThisType = ClassName; \
			using BaseType = BaseClass; \
			data::TypeDescriptor type{ #ClassName }; \
			type.Base.Id = data::GetId<BaseType>();

#define PHX_REFLECT_FIELD(FieldName) \
	type.AddField(data::FieldDescriptor::Create<ThisType, &ThisType::FieldName>(#FieldName));

#define PHX_REFLECT_END() \
			data::TypeRegistry::AddType(std::move(type)); \
		} \
	} __autoRegisterInstance_##ClassName;