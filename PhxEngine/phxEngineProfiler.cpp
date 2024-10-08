#include "pch.h"
#include "phxEngineProfiler.h"
#include  "phxSpan.h"
#include "phxSystemTime.h"
#include "EmberGfx/phxGfxDevice.h"
#include "ImGui/imgui.h"

using namespace phx;

namespace
{
    class StatHistory
    {
    public:
        StatHistory()
        {
            m_average = 0.0f;
            m_minimum = 0.0f;
            m_maximum = 0.0f;
        }

        void RecordStat(float Value)
        {
            m_recentHistory.Push(Value);
            m_extendedHistory.Push(Value);
            m_recent = Value;

            uint32_t validCount = 0;
            m_minimum = FLT_MAX;
            m_maximum = 0.0f;
            m_average = 0.0f;

            for (float val : m_recentHistory.GetSpan())
            {
                if (val > 0.0f)
                {
                    ++validCount;
                    m_average += val;
                    m_minimum = std::min(val, m_minimum);
                    m_maximum = std::max(val, m_maximum);
                }
            }

            if (validCount > 0)
                m_average /= (float)validCount;
            else
                m_minimum = 0.0f;
        }

        float GetLast(void) const { return m_recent; }
        float GetMax(void) const { return m_maximum; }
        float GetMin(void) const { return m_minimum; }
        float GetAvg(void) const { return m_average; }

        phx::Span<float> GetHistory(void) const { return this->m_extendedHistory.GetSpan(); }

    private:
        template<class T, uint32_t Size>
        struct RingBuffer
        {
            static_assert((Size & (Size - 1)) == 0, "Size must be a power of 2");

            void Push(T const& v)
            {
                this->Buffer[Tail] = v;
                Tail = (Tail + 1) & BufferMask;  // Wrap using bitwise AND

                if (Full) 
                {
                    // Move the tail to maintain the circular buffer when full
                    Head = (Head + 1) & BufferMask;
                }

                Full = (Head == Tail);
            }

            phx::Span<T> GetSpan() const { return phx::Span{ Buffer.data(), Size}; }

            std::array<T, Size> Buffer;
            uint32_t Head = 0;
            uint32_t Tail = 0;
            bool Full = false;
            const uint32_t BufferMask = (Size - 1);
        };

        RingBuffer<float, 64> m_recentHistory = {};
        RingBuffer<float, 256> m_extendedHistory = {};
        float m_recent;
        float m_average;
        float m_minimum;
        float m_maximum;
    };


    class GpuTimer
    {
    public:

        GpuTimer()
        {
            m_timerIndex = gfx::GfxDevice::CreateTimerQueryHandle();
        }

        void Start(gfx::CommandCtx* context)
        {
            context->StartTimer(this->m_timerIndex);
        }

        void Stop(gfx::CommandCtx* context)
        {
            context->EndTimer(this->m_timerIndex);
        }

        float GetTime()
        {
            return gfx::GfxDevice::GetTime(m_timerIndex);
        }

        gfx::TimerQueryHandle GetTimerIndex(void)
        {
            return m_timerIndex;
        }
    private:
        gfx::TimerQueryHandle m_timerIndex;
    };

    class TimingNode
    {
    public:
        TimingNode(std::string const& name, TimingNode* parent = nullptr)
            : m_name(name)
            , m_parent(parent)
        {

        }

        TimingNode* GetOrCreateChild(std::string const& name)
        {
            auto itr = this->m_lut.find(name);
            if (itr != this->m_lut.end())
                return itr->second;

            this->m_children.emplace_back(std::make_unique<TimingNode>(name, this));
            this->m_lut[name] = this->m_children.back().get();
            return this->m_children.back().get();
        }

        void TimingStart(gfx::CommandCtx* context)
        {
			this->m_startTick = SystemTime::GetCurrentTick();
			if (context == nullptr)
				return;

			m_gpuTimer.Start(context);

			// Context->PIXBeginEvent(m_name.c_str());
        }

        void TimingStop(gfx::CommandCtx* context)
		{
			this->m_endTick = SystemTime::GetCurrentTick();
			if (context == nullptr)
				return;

			m_gpuTimer.Stop(context);

			// Context->PIXEndEvent();
        }

        void GatherTimes()
        {
			m_cpuTime.RecordStat(1000.0f * (float)SystemTime::TimeBetweenTicks(m_startTick, m_endTick));
			m_gpuTime.RecordStat(1000.0f * m_gpuTimer.GetTime());

			for (auto& node : m_children)
				node->GatherTimes();

			m_startTick = 0;
			m_endTick = 0;
        }

        void SumInclusiveTimes(float& cpuTime, float& gpuTime)
        {
            cpuTime = 0.0f;
            gpuTime = 0.0f;
            for (auto& child : this->m_children)
            {
				cpuTime += child->m_cpuTime.GetLast();
				gpuTime += child->m_gpuTime.GetLast();
            }
        }

        TimingNode* GetParent() const { return this->m_parent; }

    private:
        const std::string m_name;
        TimingNode* m_parent;
        std::vector<std::unique_ptr<TimingNode>> m_children;
		std::unordered_map<std::string, TimingNode*> m_lut;
		StatHistory m_cpuTime;
		StatHistory m_gpuTime;
		int64_t m_startTick;
		int64_t m_endTick;
        GpuTimer m_gpuTimer;
    };

    class TimingTree final : NonCopyable
    {
    public:
        TimingTree()
	        : m_rootScope("")
        {}

        void ProfileMarkerPush(std::string const& name, gfx::CommandCtx* context)
        {
            this->m_currentNode = this->m_currentNode->GetOrCreateChild(name);
            this->m_currentNode->TimingStart(context);
        }

        void ProfileMarkerPop(gfx::CommandCtx* context)
        {
            this->m_currentNode->TimingStop(context);
            this->m_currentNode = this->m_currentNode->GetParent();
        }

        void UpdateTimes()
        {
            gfx::GfxDevice::BeginGpuTimerReadback();
            m_rootScope.GatherTimes(); 
            m_frameDelta.RecordStat(gfx::GfxDevice::GetTime(0));
            gfx::GfxDevice::EndGpuTimerReadback();

            float totalCpuTime = 0.0f;
            float totalGpuTime = 0.0f;

            this->m_rootScope.SumInclusiveTimes(totalCpuTime, totalGpuTime);
            this->m_totalCpuTime.RecordStat(totalCpuTime);
            this->m_totalGpuTime.RecordStat(totalGpuTime);
        }

		float GetTotalCpuTime() const { return this->m_totalCpuTime.GetAvg(); }
		float GetTotalGpuTime() const { return this->m_totalGpuTime.GetAvg(); }

    private:
        TimingNode m_rootScope;
        TimingNode* m_currentNode = &m_rootScope;
        StatHistory m_frameDelta;
        StatHistory m_totalCpuTime;
        StatHistory m_totalGpuTime;
    };

}

namespace
{
	TimingTree m_timingTree;

}

void phx::EngineProfile::Update()
{
	m_timingTree.UpdateTimes();
}

void phx::EngineProfile::BlockBegin(std::string const& name, gfx::CommandCtx* gfxContext)
{
    m_timingTree.ProfileMarkerPush(name, gfxContext);
}

void phx::EngineProfile::BlockEnd(gfx::CommandCtx* gfxContext)
{
	m_timingTree.ProfileMarkerPop(gfxContext);
}

void phx::EngineProfile::DrawUI()
{
    ImGui::Begin("Profiler");
	ImGui::Text("CPU time %7.3f ms", m_timingTree.GetTotalCpuTime());
	ImGui::Text("GPU time %7.3f ms", m_timingTree.GetTotalGpuTime());
    ImGui::End();
}
