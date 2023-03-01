
#include <PhxEngine/Engine/EngineRoot.h>
#include <PhxEngine/Core/TimeStep.h>
#include <PhxEngine/Core/StopWatch.h>

namespace PhxEngine
{
	namespace Core
	{
		class IWindow;
		class Event;
	}

	class PhxEngineRoot : public IPhxEngineRoot
	{
	private:
		inline static PhxEngineRoot* sSingleton = nullptr;

	public:
		PhxEngineRoot();
		~PhxEngineRoot() = default;

		void Initialize(EngineParam const& params) override;
		void Finalizing() override;

		void Run() override;
		void AddPassToBack(EngineRenderPass* pass) override;
		void RemovePass(EngineRenderPass* pass) override;

		Core::IWindow* GetWindow() override { return this->m_window.get(); }
		void SetInformativeWindowTitle(std::string_view appName, std::string_view extraInfo) override;

		RHI::IGraphicsDevice* GetGfxDevice() override { return this->m_gfxDevice.get(); }
		virtual const DirectX::XMFLOAT2& GetCanvasSize() const override{ return this->m_canvasSize; }
		std::shared_ptr<Core::FrameProfiler> GetFrameProfiler() override { return this->m_frameProfiler; }

	private:
		void Update(Core::TimeStep const& deltaTime);
		void Render();
		void ProcessEvent(Core::Event& e);

	private:
		Core::StopWatch m_frameTimer; // TODO: Move to a profiler and scope
		EngineParam m_params;

		// Engine Passes
		std::vector<EngineRenderPass*> m_renderPasses;

		// Engine Core Systems
		// Move logging system here.
		std::shared_ptr<Core::FrameProfiler> m_frameProfiler;
		std::unique_ptr<RHI::IGraphicsDevice> m_gfxDevice;
		std::unique_ptr<Core::IWindow> m_window;
		DirectX::XMFLOAT2 m_canvasSize;
		bool m_windowIsVisibile;
		uint64_t m_frameCount;

		class SimpleProfiler
		{
		public:
			double AverageFrameTime = 0.0f;

			void UpdateAverageFrameTime(double elapsedTime)
			{
				this->m_frameTimeSum += elapsedTime;
				this->m_numAccumulatedFrames += 1;

				if (this->m_frameTimeSum > this->m_averageTimeUpdateInterval && this->m_numAccumulatedFrames > 0)
				{
					this->AverageFrameTime = m_frameTimeSum / double(this->m_numAccumulatedFrames);
					this->m_numAccumulatedFrames = 0;
					this->m_frameTimeSum = 0.0;
				}
			}

		private:
			double m_frameTimeSum = 0.0f;
			uint64_t m_numAccumulatedFrames = 0;
			double m_averageTimeUpdateInterval = 0.5f;
		} m_profile = {};
	};
}