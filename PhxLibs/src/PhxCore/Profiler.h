#pragma once

#include <tracy/Tracy.hpp>

#define PHX_PROFILE ZoneScoped
#define PHX_PROFILE_FRAME FrameMark
#define PHX_PROFILE_SECTION(x) ZoneScopedN(x)
#define PHX_PROFILE_TAG(y, x) ZoneText(x, strlen(x))
#define PHX_PROFILE_LOG(text, size) TracyMessage(text, size)
#define PHX_PROFILE_VALUE(text, value) TracyPlot(text, value)

#if false
namespace phx
{
	namespace Profiler
	{
#if false
		void Update();
		void BlockBegin(std::string const& name, gfx::CommandCtx* gfxContext = nullptr);
		void BlockEnd(gfx::CommandCtx* gfxContext = nullptr);

		void DrawUI();
#else

		inline void Update() {};
		inline void BlockBegin(std::string const& name, gfx::CommandCtx* gfxContext = nullptr) {};
		inline void BlockEnd(gfx::CommandCtx* gfxContext = nullptr) {};

		inline void DrawUI() {};
#endif
	}

	class ScopedBlock
	{
	public:
		ScopedBlock(std::string const& funcName)
			: m_gfxContext(nullptr)
		{
				Profiler::BlockBegin(funcName);
		}

		ScopedBlock(std::string const& funcName, std::string const&  override)
			: m_gfxContext(nullptr)
		{
			if (override.empty() == 0)
				Profiler::BlockBegin(override);
			else
				Profiler::BlockBegin(funcName);
		}

		ScopedBlock(std::string const& funcName, gfx::CommandCtx* gfxContext)
			: m_gfxContext(gfxContext)
		{
				Profiler::BlockBegin(funcName, gfxContext);
		}

		~ScopedBlock()
		{
			Profiler::BlockEnd(this->m_gfxContext);
		}

	private:
		rhi::CommandCtx* m_gfxContext;
	};
}

#define PHX_EVENT() phx::ScopedBlock scope(__func__)
#define PHX_EVENT_GFX(ctx) phx::ScopedBlock scope(__func__, &ctx)
#endif