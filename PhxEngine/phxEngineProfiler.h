#pragma once

#include "EmberGfx/phxGfxCommandCtx.h"

namespace phx
{
	namespace EngineProfile
	{
		void Update();
		void BlockBegin(std::string const& name, gfx::CommandCtx* gfxContext = nullptr);
		void BlockEnd(gfx::CommandCtx* gfxContext = nullptr);

		void DrawUI();
	}

	class ScopedBlock
	{
	public:
		ScopedBlock(std::string const& funcName, std::string const&  override)
			: m_gfxContext(nullptr)
		{
			if (override.empty() == 0)
				EngineProfile::BlockBegin(override);
			else
				EngineProfile::BlockBegin(funcName);
		}

		ScopedBlock(std::string const&  name, gfx::CommandCtx* gfxContext)
			: m_gfxContext(gfxContext)
		{
			EngineProfile::BlockBegin(name);
		}

		~ScopedBlock()
		{
			EngineProfile::BlockEnd(this->m_gfxContext);
		}

	private:
		gfx::CommandCtx* m_gfxContext;
	};
}

#define PHX_EVENT(...) ScopedBlock scopedLock(__func__, ##__VA_A