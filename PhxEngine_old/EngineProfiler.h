#pragma once

#include "EmberGfx/phxEmber.h"

namespace phx
{
	namespace EngineProfile
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
				EngineProfile::BlockBegin(funcName);
		}

		ScopedBlock(std::string const& funcName, std::string const&  override)
			: m_gfxContext(nullptr)
		{
			if (override.empty() == 0)
				EngineProfile::BlockBegin(override);
			else
				EngineProfile::BlockBegin(funcName);
		}

		ScopedBlock(std::string const& funcName, gfx::CommandCtx* gfxContext)
			: m_gfxContext(gfxContext)
		{
				EngineProfile::BlockBegin(funcName, gfxContext);
		}

		~ScopedBlock()
		{
			EngineProfile::BlockEnd(this->m_gfxContext);
		}

	private:
		gfx::CommandCtx* m_gfxContext;
	};
}

#define PHX_EVENT() phx::ScopedBlock scope(__func__)
#define PHX_EVENT_GFX(ctx) phx::ScopedBlock scope(__func__, &ctx)