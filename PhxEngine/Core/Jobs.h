#pragma once

#include <functional>
#include <memory>

namespace phx::Jobs
{
    // Number of worker threads (0 = hardware_concurrency). Must be called
    // once before any Graph is run — see Engine::Initialize.
    //
    // on_worker_start/on_worker_stop, if given, run once on each worker
    // thread itself -- at that thread's start-up (before it processes any
    // task) and shutdown (after it's done processing tasks) respectively.
    // Meant for other systems that need their own per-thread state set up
    // wherever Jobs happens to run work (e.g. Memory::InitializeThreadLocal
    // for Memory::GetFrameAlloc/GetScratchAlloc) without Jobs itself
    // needing to know anything about them.
    void Initialize(u32 thread_count = 0,
        std::function<void()> on_worker_start = nullptr,
        std::function<void()> on_worker_stop  = nullptr);
    void Shutdown();

    u32 GetWorkerCount();

    // Stable per-thread slot for the calling thread: 0..GetWorkerCount()-1
    // if called from inside a running task (a Jobs worker thread), or
    // GetWorkerCount() (one past the end) from any other thread. Meant for
    // giving each possible caller its own per-thread resource -- e.g. RHI's
    // command pools, which Vulkan requires be externally synchronized (one
    // thread at a time) -- without exposing Taskflow's own worker-id API.
    u32 GetCurrentThreadSlot();

    // Opaque reference to a node within the Graph that created it — only
    // meaningful passed back into that same Graph's Precede() before the
    // next Clear().
    struct TaskHandle
    {
        u32 index = ~0u;
    };

    // A graph of task nodes. Meant to be a single long-lived instance (e.g.
    // one frame graph, owned by whoever drives the frame) that gets reused
    // every frame:
    //   Wait(graph);   // previous frame's run must be done first
    //   graph.Clear();
    //   ...Emplace/ForEachIndex/Precede...
    //   Run(graph);
    // Do NOT construct a fresh Graph every frame — construction allocates
    // once; Clear() does not.
    class Graph
    {
    public:
        Graph();
        ~Graph();
        Graph(Graph&&) noexcept;
        Graph& operator=(Graph&&) noexcept;
        Graph(const Graph&) = delete;
        Graph& operator=(const Graph&) = delete;

        TaskHandle Emplace(std::function<void()> fn);

        // Data-parallel task over [begin, end), auto-partitioned across workers.
        TaskHandle ForEachIndex(u32 begin, u32 end, std::function<void(u32)> fn);

        void Precede(TaskHandle before, TaskHandle after);

        // Embeds `subgraph` as a single node -- when reached, the whole of
        // `subgraph` runs to completion before this node is considered
        // done. `subgraph` is referenced, not copied: it must stay alive
        // and must not be Clear()'d/rebuilt while this graph might still
        // be running (Wait() this graph first). Typical use: build several
        // independent sections (e.g. one per app subsystem) as their own
        // long-lived Graphs, then ComposeOf() them into a parent graph
        // that only defines the ordering between sections.
        TaskHandle ComposeOf(Graph& subgraph);

        // Wipes the graph back to empty so it can be rebuilt and rerun —
        // call this every frame instead of constructing a new Graph. Must
        // not have a Run() in flight (Wait() first; asserted).
        void Clear();

    private:
        struct Impl;
        std::unique_ptr<Impl> m_impl;

        friend void Run(Graph&);
        friend void Wait(Graph&);
    };

    // Runs every task in the graph. Non-blocking; call Wait() (or use
    // RunAndWait) to block until done. `graph` must outlive the run.
    void Run(Graph& graph);
    void Wait(Graph& graph);
    inline void RunAndWait(Graph& graph) { Run(graph); Wait(graph); }
}
