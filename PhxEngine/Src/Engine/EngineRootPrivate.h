
#include <PhxEngine/Engine/EngineRoot.h>
#include <PhxEngine/Core/TimeStep.h>
#include <PhxEngine/Core/StopWatch.h>

namespace PhxEngine
{
	namespace Core
	{
		class IWindow;
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

		RHI::IGraphicsDevice* GetGfxDevice() override { return this->m_gfxDevice.get(); }

	private:
		void Update(Core::TimeStep const& deltaTime);
		void Render();

	private:
		Core::StopWatch m_frameTimer; // TODO: Move to a profiler and scope
		EngineParam m_params;

		// Engine Passes
		std::vector<EngineRenderPass*> m_renderPasses;

		// Engine Core Systems
		// Move logging system here.
		std::unique_ptr<RHI::IGraphicsDevice> m_gfxDevice;
		std::unique_ptr<Core::IWindow> m_window;
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
					this->AverageFrameTime = m_frameTimeSum / double(m_averageTimeUpdateInterval);
					this->m_numAccumulatedFrames = 0;
					m_frameTimeSum = 0.0;
				}
			}

		private:
			double m_frameTimeSum = 0.0f;
			uint64_t m_numAccumulatedFrames = 0;
			double m_averageTimeUpdateInterval = 0.0f;
		} m_profile = {};
	};
}