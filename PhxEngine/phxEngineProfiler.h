#pragma once

#include "EmberGfx/phxGfxDeviceResources.h"
#include "EmberGfx/phxGfxCommandCtx.h"
#include "phxStringHash.h"

namespace phx
{
	namespace EngineProfile
	{
		void Update();
		void BlockBegin(StringHash name, gfx::CommandCtx* gfxContext = nullptr);
		void BlockEnd(gfx::CommandCtx* gfxContext = nullptr);

		void DrawUI();
	}

	class ScopedBlock
	{
	public:
		ScopedBlock(StringHash funcName, StringHash override)
			: m_gfxContext(nullptr)
		{
			if (override.Value() == 0)
				EngineProfile::BlockBegin(override);
			else
				EngineProfile::BlockBegin(funcName);
		}

		ScopedBlock(StringHash name, gfx::CommandCtx* gfxContext)
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

#define PHX_EVENT(...) ScopedBlock scopedLock(__func__, ##__VA_ARGS__);