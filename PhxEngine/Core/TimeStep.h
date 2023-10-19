#pragma once

namespace PhxEngine::Core
{
	class TimeStep
	{
	public:
		TimeStep(float time = 0.0f)
			: m_time(time)
		{
		}

		operator float() const { return this->m_time; }

		float GetSeconds() const { return this->m_time; }
		float GetMilliseconds() const { return this->m_time * 1000.0f; }

	private:
		float m_time = 0.0f;
	};
}