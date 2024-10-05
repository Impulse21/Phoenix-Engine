#include "pch.h"
#include "phxEngineProfiler.h"
#include  "phxSpan.h"

namespace
{

    class StatHistory
    {
    public:
        StatHistory()
        {
            for (uint32_t i = 0; i < kHistorySize; ++i)
                m_RecentHistory[i] = 0.0f;
            for (uint32_t i = 0; i < kExtendedHistorySize; ++i)
                m_ExtendedHistory[i] = 0.0f;
            m_Average = 0.0f;
            m_Minimum = 0.0f;
            m_Maximum = 0.0f;
        }

        void RecordStat(uint32_t FrameIndex, float Value)
        {
            m_RecentHistory[FrameIndex % kHistorySize] = Value;
            m_ExtendedHistory[FrameIndex % kExtendedHistorySize] = Value;
            m_Recent = Value;

            uint32_t ValidCount = 0;
            m_Minimum = FLT_MAX;
            m_Maximum = 0.0f;
            m_Average = 0.0f;

            for (float val : m_RecentHistory)
            {
                if (val > 0.0f)
                {
                    ++ValidCount;
                    m_Average += val;
                    m_Minimum = std::min(val, m_Minimum);
                    m_Maximum = std::max(val, m_Maximum);
                }
            }

            if (ValidCount > 0)
                m_Average /= (float)ValidCount;
            else
                m_Minimum = 0.0f;
        }

        float GetLast(void) const { return m_Recent; }
        float GetMax(void) const { return m_Maximum; }
        float GetMin(void) const { return m_Minimum; }
        float GetAvg(void) const { return m_Average; }

        phx::Span<float> GetHistory(void) const { return { m_ExtendedHistory, kExtendedHistorySize }; }

    private:
        static const uint32_t kHistorySize = 64;
        static const uint32_t kExtendedHistorySize = 256;
        float m_RecentHistory[kHistorySize];
        float m_ExtendedHistory[kExtendedHistorySize];
        float m_Recent;
        float m_Average;
        float m_Minimum;
        float m_Maximum;
    };
}


namespace
{

}

void phx::EngineProfile::Update()
{
}

void phx::EngineProfile::BlockBegin(StringHash name, gfx::CommandCtx* gfxContext)
{
}

void phx::EngineProfile::BlockEnd(gfx::CommandCtx* gfxContext)
{
}

void phx::EngineProfile::DrawUI()
{
}
