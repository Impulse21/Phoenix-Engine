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

		ScopedBlock(std::string const& funcName, std::string const&  override, gfx::CommandCtx* gfxContext)
			: m_gfxContext(gfxContext)
		{
			if (override.empty() == 0)
				EngineProfile::BlockBegin(override, gfxContext);
			else
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

#define PHX_EVENT(...) phx::ScopedBlock scope(__func__, ##__VA_ARGS__)