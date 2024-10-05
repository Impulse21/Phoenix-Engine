#include "pch.h"
#include "phxEngineProfiler.h"
#include  "phxSpan.h"

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
                this->Buffer[Tail] = item;
                Tail = (Tail + 1) & BufferMAsk;  // Wrap using bitwise AND

                if (Full) 
                {
                    // Move the tail to maintain the circular buffer when full
                    Head = (Head + 1) & BufferMask;
                }

                Full = (head == tail);
            }

            phx::Span<T> GetSpan() const { return { Buffer.data, Size }; }

            std::array<T, Size> Buffer;
            uint32_t Head = 0;
            uint32_t Tail = 0;
            bool Full = false;
            const uint32_t BufferMask = (size - 1);
        };

        RingBuffer<float, 64> m_recentHistory = {};
        RingBuffer<float, 256> m_extendedHistory = {};
        float m_recent;
        float m_average;
        float m_minimum;
        float m_maximum;
    };


    class TimingNode
    {
    public:
        
    private:
        std::vector<std::unique_ptr<TimingNode>> m_children;
        std::unordered_map<StringHash, TimingNode*> m_lut;
    };
    class TimingTree
    {
    public:
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
