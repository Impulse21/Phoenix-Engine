#pragma once

#include <taskflow/taskflow.hpp>

namespace phx
{
	using Subflow = tf::Subflow;
	class IApplication
	{
	public:
		virtual void OnStartup() = 0;
		virtual void OnShutdown() = 0;

		virtual void OnPreRender(Subflow* subflow = nullptr) = 0;
		virtual void OnRender(Subflow* subflow = nullptr) = 0;
		virtual void OnUpdate(Subflow* subflow = nullptr) = 0;

		virtual bool EnableMultiThreading() = 0;

		virtual ~IApplication() = default;
	};
}

