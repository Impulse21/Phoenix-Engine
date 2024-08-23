#pragma once

namespace phx
{
	class World;

	class IRenderSystem
	{
	public:
		virtual ~IRenderSystem() = default;

		virtual void* CacheData(World const& world) = 0;

		virtual void Render() = 0;

	};

	class RenderSystemMesh : public IRenderSystem
	{
	public:
		RenderSystemMesh() = default;
		~RenderSystemMesh() override = default;

		void* CacheData(World const& world) override;
		void Render() override;

	private:
	};
}