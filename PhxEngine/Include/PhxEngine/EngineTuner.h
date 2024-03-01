#pragma once

#include <json.hpp>

#include <limits>

namespace nlohmann
{
	class Json;
}

namespace PhxEngine
{

	class VariableGroup;
	class EngineVar
	{
	public:
		virtual ~EngineVar() = default;

		virtual void Increment() { this->m_actionCallback(ActionType::Increment); };
		virtual void Decrement() { this->m_actionCallback(ActionType::Decrement); };
		virtual void Toggle() { this->m_actionCallback(ActionType::Toggle); };

	public:
		enum class ActionType
		{
			Increment,
			Decrement,
			Toggle
		};

		using ActionCallback = std::function<void(ActionType)>;

	protected:
		EngineVar() = default;
		EngineVar(const std::string& path, ActionCallback pfnCallback = DefaultActionHandler);

		static void DefaultActionHandler(ActionType)
		{
			// no-op
		}

	private:
		friend class VariableGroup;
		VariableGroup* m_groupPtr = nullptr;

		ActionCallback m_actionCallback;
	};


	class BoolVar : public EngineVar
	{
	public:
		BoolVar(const std::string& path, bool val, ActionCallback pfnCallback = EngineVar::DefaultActionHandler);
		BoolVar& operator=(bool val) { this->m_flag = val; return *this; }
		operator bool() const { return this->m_flag; }

		virtual void Increment() override { this->m_flag = true; EngineVar::Increment(); }
		virtual void Decrement() override { this->m_flag = false; EngineVar::Decrement(); }
		virtual void Toggle() override { this->m_flag = !this->m_flag;  EngineVar::Toggle(); }
		

	private:
		bool m_flag;
	};

	class FloatVar : public EngineVar
	{
	public:
		FloatVar(std::string const& path, float val, float minValue = std::numeric_limits<float>::min(), float maxValue = std::numeric_limits<float>::max(), float stepSize = 1.0f, ActionCallback pfnCallback = EngineVar::DefaultActionHandler);
		FloatVar& operator=(float val) { this->m_value = Clamp(val); return *this; }
		operator float() const { return this->m_value; }

		virtual void Increment() override { this->m_value = Clamp(this->m_value + this->m_stepSize); EngineVar::Increment(); }
		virtual void Decrement() override { this->m_value = Clamp(this->m_value - this->m_stepSize); EngineVar::Decrement(); }

	protected:
		float Clamp(float val) { return val > this->m_maxValue ? this->m_maxValue : val < this->m_minValue ? this->m_minValue : val; }

		float m_value;
		float m_minValue;
		float m_maxValue;
		float m_stepSize;
	};

	class IntVar : public EngineVar
	{
	public:
		IntVar(const std::string& path, int32_t val, int32_t minValue = 0, int32_t maxValue = (1 << 24) - 1, int32_t stepSize = 1, ActionCallback pfnCallback = EngineVar::DefaultActionHandler);
		IntVar& operator=(int32_t val) { this->m_value = Clamp(val); return *this; }
		operator int32_t() const { return this->m_value; }

		virtual void Increment() override { this->m_value = this->Clamp(this->m_value + this->m_stepSize); EngineVar::Increment(); }
		virtual void Decrement() override { this->m_value = this->Clamp(this->m_value - this->m_stepSize); EngineVar::Decrement(); }

	protected:
		int32_t Clamp(int32_t val) { return val > this->m_maxValue ? this->m_maxValue : val < this->m_minValue ? this->m_minValue : val; }

		int32_t m_value;
		int32_t m_minValue;
		int32_t m_maxValue;
		int32_t m_stepSize;
	};

	class SizeVar : public EngineVar
	{
	public:
		SizeVar(const std::string& path, size_t val, size_t minValue = std::numeric_limits<size_t>::min(), size_t maxValue = std::numeric_limits<size_t>::max(), size_t stepSize = 1, ActionCallback pfnCallback = EngineVar::DefaultActionHandler);
		SizeVar& operator=(int32_t val) { this->m_value = Clamp(val); return *this; }
		operator size_t() const { return this->m_value; }

		virtual void Increment() override { this->m_value = this->Clamp(this->m_value + this->m_stepSize); EngineVar::Increment(); }
		virtual void Decrement() override { this->m_value = this->Clamp(this->m_value - this->m_stepSize); EngineVar::Decrement(); }

	protected:
		size_t Clamp(size_t val) { return val > this->m_maxValue ? this->m_maxValue : val < this->m_minValue ? this->m_minValue : val; }

		size_t m_value;
		size_t m_minValue;
		size_t m_maxValue;
		size_t m_stepSize;
	};

	namespace EngineTuner
	{
		void Startup();
		void SetValues(nlohmann::Json const& data);
		void Shutdown();
	}
}