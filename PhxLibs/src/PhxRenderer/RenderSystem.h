#pragma once

#include <memory>
#include <PhxRenderer/RenderLayer.h>

namespace phx
{
	class World;
	namespace rhi
	{
		class CommandCtx;
	}
}

namespace phx::gfx
{
	template<typename T>
	concept RenderSubSystemType = std::is_base_of_v<IRenderSubSystem, T>;

	class IRenderSystem
	{
	public:
		inline static IRenderSystem* Ptr = nullptr;

	public:
		virtual ~IRenderSystem() = default;

		virtual void Initialize() = 0;
		virtual void Finalize() = 0;

		virtual void RegisterObservers(phx::World& world) = 0;

		virtual void OnPreRender(World& world) = 0;
		virtual void OnRender() = 0;

		virtual void RegisterSubSystem(uint32_t passMask, std::shared_ptr<IRenderSubSystem> subSystem) = 0;

		template<RenderSubSystemType TSubSystem>
		void RegisterSubSystem(uint32_t passMask)
		{
			RegisterSubSystem(passMask, std::make_shared<TSubSystem>());
		}
	};
}