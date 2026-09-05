#include "Jobs.h"

#include <PhxEngine/Core/CVar.h>

#include <taskflow/taskflow.hpp>
#include <taskflow/algorithm/for_each.hpp>

#if defined(PHX_PROFILING_ENABLED)
    #include <tracy/Tracy.hpp>
#endif

#include <chrono>
#include <future>
#include <string>
#include <thread>
#include <vector>

PHX_CVAR_INT(jobs_thread_count, 0, "Jobs worker thread count (0 = hardware_concurrency)");

namespace
{
    std::unique_ptr<tf::Executor> g_executor;

    // Wraps plain std::function callbacks as a tf::WorkerInterface so
    // Jobs.h never has to name that Taskflow type -- see Jobs::Initialize.
    struct CallbackWorkerInterface : tf::WorkerInterface
    {
        std::function<void()> on_start;
        std::function<void()> on_stop;

        void scheduler_prologue(tf::Worker&) override
        {
            if (on_start)
                on_start();
        }

        void scheduler_epilogue(tf::Worker&, std::exception_ptr) override
        {
            if (on_stop)
                on_stop();
        }
    };

#if defined(PHX_PROFILING_ENABLED)
    struct TracyObserver : public tf::ObserverInterface
    {
        void set_up(size_t /*num_workers*/) override
        {
        }

        void on_entry(tf::WorkerView wv, tf::TaskView /*task_view*/) override
        {
            // on_entry fires on every single task, but a thread's name only
            // ever needs setting once -- thread_local so each worker OS
            // thread names itself exactly once, on its first task, instead
            // of re-registering the same name on every task it ever runs.
            thread_local bool s_named = false;
            if (!s_named)
            {
                const std::string thread_name = "Taskflow Worker " + std::to_string(wv.id());
                tracy::SetThreadName(thread_name.c_str());
                s_named = true;
            }
        }

        void on_exit(tf::WorkerView /*wv*/, tf::TaskView /*task_view*/) override
        {
            // Optional: logic when a worker finishes a task
        }
    };
#endif
}  // namespace

struct phx::Jobs::Graph::Impl
{
    tf::Taskflow          taskflow;
    std::vector<tf::Task> tasks;
    tf::Future<void>      future;
};

phx::Jobs::Graph::Graph()
    : m_impl(std::make_unique<Impl>())
{
}

phx::Jobs::Graph::~Graph() = default;
phx::Jobs::Graph::Graph(Graph&&) noexcept = default;
phx::Jobs::Graph& phx::Jobs::Graph::operator=(Graph&&) noexcept = default;

phx::Jobs::TaskHandle phx::Jobs::Graph::Emplace(std::function<void()> fn)
{
    m_impl->tasks.push_back(m_impl->taskflow.emplace(std::move(fn)));
    return TaskHandle{ static_cast<u32>(m_impl->tasks.size() - 1) };
}

phx::Jobs::TaskHandle phx::Jobs::Graph::ForEachIndex(u32 begin, u32 end, std::function<void(u32)> fn)
{
    m_impl->tasks.push_back(m_impl->taskflow.for_each_index(begin, end, 1u, std::move(fn)));
    return TaskHandle{ static_cast<u32>(m_impl->tasks.size() - 1) };
}

void phx::Jobs::Graph::Precede(TaskHandle before, TaskHandle after)
{
    m_impl->tasks[before.index].precede(m_impl->tasks[after.index]);
}

phx::Jobs::TaskHandle phx::Jobs::Graph::ComposeOf(Graph& subgraph)
{
    m_impl->tasks.push_back(m_impl->taskflow.composed_of(subgraph.m_impl->taskflow));
    return TaskHandle{ static_cast<u32>(m_impl->tasks.size() - 1) };
}

void phx::Jobs::Graph::Clear()
{
    // future.valid() alone doesn't mean "still running" -- it stays true
    // after a completed wait() too, only going false on get()/move. Check
    // actual readiness instead.
    PHX_ASSERT((!m_impl->future.valid() || m_impl->future.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
        && "Jobs::Graph::Clear: previous Run() still in flight — Wait() first");

    m_impl->taskflow.clear(); // drops nodes, keeps internal storage capacity
    m_impl->tasks.clear();    // keeps std::vector capacity
    m_impl->future = {};
}

void phx::Jobs::Initialize(u32 thread_count, std::function<void()> on_worker_start, std::function<void()> on_worker_stop)
{
    const u32 count = thread_count != 0
        ? thread_count
        : (CVar_jobs_thread_count.Get() != 0
            ? static_cast<u32>(CVar_jobs_thread_count.Get())
            : std::thread::hardware_concurrency());

    if (on_worker_start || on_worker_stop)
    {
        auto wif = std::make_shared<CallbackWorkerInterface>();
        wif->on_start = std::move(on_worker_start);
        wif->on_stop  = std::move(on_worker_stop);
        g_executor = std::make_unique<tf::Executor>(count, std::move(wif));
    }
    else
    {
        g_executor = std::make_unique<tf::Executor>(count);
    }

#if defined(PHX_PROFILING_ENABLED)
    g_executor->make_observer<TracyObserver>();
#endif
}

void phx::Jobs::Shutdown()
{
    g_executor.reset(); // ~Executor() drains + joins workers
}

u32 phx::Jobs::GetWorkerCount()
{
    return g_executor ? static_cast<u32>(g_executor->num_workers()) : 0;
}

u32 phx::Jobs::GetCurrentThreadSlot()
{
    if (!g_executor)
        return 0;

    const int worker_id = g_executor->this_worker_id(); // -1 if not a worker of g_executor
    return worker_id < 0 ? static_cast<u32>(g_executor->num_workers()) : static_cast<u32>(worker_id);
}

void phx::Jobs::Run(Graph& graph)
{
    PHX_ASSERT(g_executor);
    graph.m_impl->future = g_executor->run(graph.m_impl->taskflow);
}

void phx::Jobs::Wait(Graph& graph)
{
    if (graph.m_impl->future.valid())
        graph.m_impl->future.wait();
}
