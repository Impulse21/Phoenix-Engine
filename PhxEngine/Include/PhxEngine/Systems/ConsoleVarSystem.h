#pragma once

#include "PhxEngine/Core/StringHash.h"
#include "PhxEngine/Engine/Layer.h"

// Credit for this code goes to: https://vkguide.dev/docs/extra-chapter/cvar_system/
// What a great little tutorial

namespace PhxEngine
{
	enum class ConsoleVarFlags : uint32_t
	{
		None = 0,
		NoEdit = 1 << 1,
		EditReadOnly = 1 << 2,
		Advanced = 1 << 3,

		EditCheckbox = 1 << 8,
		EditFloatDrag = 1 << 8,
	};

	struct ConsoleVarParameter;


	class ConsoleVarSystem
	{
	public:
		static ConsoleVarSystem* GetInstance();

	public:
		virtual double* GetFloatConsoleVar(Core::StringHash hash) = 0;
		virtual void SetFloatConsoleVar(Core::StringHash hash, double value) = 0;

		virtual int32_t* GetIntConsoleVar(Core::StringHash hash) = 0;
		virtual void SetIntConsoleVar(Core::StringHash hash, int32_t value) = 0;

		virtual const char* GetStringConsoleVar(Core::StringHash hash) = 0;
		virtual void SetStringConsoleVar(Core::StringHash hash, const char* value) = 0;

		virtual void DrawImguiEditor() = 0;

	public:
		virtual ConsoleVarParameter* CreateFloatConsoleVar(const char* name, const char* description, double defaultValue, double currentValue) = 0;
		virtual ConsoleVarParameter* CreateIntConsoleVar(const char* name, const char* description, int32_t defaultValue, int32_t currentValue) = 0;
		virtual ConsoleVarParameter* CreateStringConsoleVar(const char* name, const char* description, const char* defaultValue, const char* currentValue) = 0;
	};

	template<typename T>
	struct AutoCConsoleVar
	{
	protected:
		int index;
		using ConsoleVarType = T;
	};

	struct AutoConsoleVar_Float : AutoCConsoleVar<double>
	{
		AutoConsoleVar_Float(const char* name, const char* description, double defaultValue, ConsoleVarFlags flags = ConsoleVarFlags::None);

		double Get();
		double* GetPtr();
		float GetFloat();
		float* GetFloatPtr();
		void Set(double val);
	};

	struct AutoConsoleVar_Int : AutoCConsoleVar<int32_t>
	{
		AutoConsoleVar_Int(const char* name, const char* description, int32_t defaultValue, ConsoleVarFlags flags = ConsoleVarFlags::None);
		int32_t Get();
		int32_t* GetPtr();
		void Set(int32_t val);

		void Toggle();
	};

	struct AutoConsoleVar_String : AutoCConsoleVar<std::string>
	{
		AutoConsoleVar_String(const char* name, const char* description, const char* defaultValue, ConsoleVarFlags flags = ConsoleVarFlags::None);

		const char* Get();
		void Set(std::string&& val);
	};

	class ConsoleVarAppLayer : public AppLayer
	{
	public:
		ConsoleVarAppLayer()
			: AppLayer("Console Var Layer")
		{};

		void OnRenderImGui() override { ConsoleVarSystem::GetInstance()->DrawImguiEditor(); }
	};
}

