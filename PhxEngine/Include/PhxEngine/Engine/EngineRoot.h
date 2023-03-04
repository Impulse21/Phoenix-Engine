#pragma once
#include <memory>
#include <string>
#include <PhxEngine/RHI/PhxRHI.h>
#include <PhxEngine/Engine/ApplicationEvents.h>
#include <PhxEngine/Core/Profiler.h>
#include <DirectXMath.h>

namespace PhxEngine
{
	namespace Core
	{
		class IWindow;
	}

	struct EngineParam
	{
		RHI::GraphicsAPI GraphicsAPI = RHI::GraphicsAPI::Unknown;
		uint32_t FramesInFlight = 3;

		std::string Name = "";
		uint32_t WindowWidth = 1280;
		uint32_t WindowHeight = 720;
		bool FullScreen = false;
		bool VSync = false;
		bool EnableImGui = false;
		bool AllowWindowResize = true;
	};

	class EngineRenderPass;
	class IPhxEngineRoot
	{
	public:
		virtual ~IPhxEngineRoot() = default;

		virtual void Initialize(EngineParam const& params) = 0;
		virtual void Finalizing() = 0;

		virtual void Run() = 0;
		virtual void AddPassToBack(EngineRenderPass* pass) = 0;
		virtual void RemovePass(EngineRenderPass* pass) = 0;

		virtual RHI::IGraphicsDevice* GetGfxDevice() = 0;
		virtual const DirectX::XMFLOAT2& GetCanvasSize() const = 0;
		virtual void SetInformativeWindowTitle(std::string_view appName, std::string_view extraInfo) = 0;

		virtual std::shared_ptr<Core::FrameProfiler> GetFrameProfiler() = 0;
		virtual Core::IWindow* GetWindow() = 0;
	};

	class EngineRenderPass
	{
	public:
		explicit EngineRenderPass(IPhxEngineRoot* root)
			: m_root(root) {}

		virtual void Update(Core::TimeStep const& delta) {};
		virtual void Render() {};

		virtual void OnWindowResize(WindowResizeEvent const& resizeEvent) {};

	public:
		IPhxEngineRoot* GetRoot() { return this->m_root; }
		RHI::IGraphicsDevice* GetGfxDevice() { return this->m_root->GetGfxDevice(); }

	private:
		IPhxEngineRoot* m_root;
	};


	std::unique_ptr<IPhxEngineRoot> CreateEngineRoot();
}