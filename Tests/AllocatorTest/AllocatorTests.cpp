// Author: Claude Code
// AllocatorTests.cpp
//
// Framework-free smoke tests for the allocator cores (BumpCore, SlotCore,
// VirtualHeapCore), the allocators built on them that don't need a GPU
// device (GpuBumpAllocator over a fake range), and the CPU memory system
// (VirtualPageSource, FrameArena, Frame/Scratch LinearAllocators, and the
// Memory:: thread-local API driven from Jobs workers).
// Returns 0 on success, 1 if any CHECK failed.
//
// Everything here must run without an RHI: VirtualHeapCore is pure CPU
// bookkeeping (a VMA virtual block needs no VkDevice).

#include <PhxEngine/Core/PhxDefines.h>
#include <PhxEngine/Core/Jobs.h>
#include <PhxEngine/Core/Log.h>
#include <PhxEngine/Memory/BumpCore.h>
#include <PhxEngine/Memory/SlotCore.h>
#include <PhxEngine/Memory/LinearAllocator.h>
#include <PhxEngine/Memory/MemoryHelpers.h>
#include <PhxEngine/Memory/VirtualPageSource.h>
#include <PhxEngine/Memory/FrameArena.h>
#include <PhxEngine/RHI/GpuMemory/BumpAllocator.h>
#include <PhxEngine/RHI/GpuMemory/VirtualHeapCore.h>

#include <atomic>
#include <barrier>
#include <cstdio>
#include <cstring>
#include <thread>
#include <utility>
#include <vector>

using namespace phx;

namespace
{
    int g_failures = 0;

#define CHECK(cond)                                                                  \
    do {                                                                             \
        if (!(cond))                                                                 \
        {                                                                            \
            std::printf("    CHECK FAILED %s:%d  %s\n", __FILE__, __LINE__, #cond);  \
            ++g_failures;                                                            \
        }                                                                            \
    } while (0)

    template<typename TFn>
    void Run(const char* name, TFn&& body)
    {
        std::printf("[ RUN  ] %s\n", name);
        const int before = g_failures;
        body();
        std::printf(g_failures == before ? "[  OK  ] %s\n" : "[ FAIL ] %s\n", name);
    }

    bool IsAligned(u64 value, u64 alignment) { return (value & (alignment - 1)) == 0; }
    bool IsAligned(const void* ptr, u64 alignment) { return IsAligned(reinterpret_cast<uptr>(ptr), alignment); }

    // -- BumpCore ---

    void TestBumpAlignsOffsetNotSize()
    {
        // The Phase 1 bug: Alloc(4,16) then Alloc(n,256) used to return 16.
        BumpCore core(4096);
        CHECK(core.Alloc(4, 16) == 0);
        const u64 second = core.Alloc(8, 256);
        CHECK(second == 256);
        CHECK(IsAligned(second, 256));
        CHECK(core.Used() == 256 + 8);
    }

    void TestBumpExactFitAndOverflow()
    {
        BumpCore core(64);
        CHECK(core.Alloc(64, 1) == 0);
        CHECK(core.Remaining() == 0);
        CHECK(core.Alloc(1, 1) == BumpCore::kInvalidOffset);
        CHECK(core.Used() == 64);  // failed alloc leaves state untouched
    }

    void TestBumpPaddingOverflow()
    {
        // Fits by size, but not after alignment padding.
        BumpCore core(100);
        CHECK(core.Alloc(10, 1) == 0);
        CHECK(core.Alloc(80, 64) == BumpCore::kInvalidOffset);
        CHECK(core.Used() == 10);

        // Padding alone past the end.
        BumpCore small(70);
        CHECK(small.Alloc(66, 1) == 0);
        CHECK(small.Alloc(0, 128) == BumpCore::kInvalidOffset);
    }

    void TestBumpHugeSizeDoesNotWrap()
    {
        BumpCore core(4096);
        CHECK(core.Alloc(16, 1) == 0);
        CHECK(core.Alloc(~u64(0) - 8, 1) == BumpCore::kInvalidOffset);
        CHECK(core.Used() == 16);
    }

    void TestBumpZeroSize()
    {
        BumpCore core(256);
        CHECK(core.Alloc(3, 1) == 0);
        CHECK(core.Alloc(0, 16) == 16);   // consumes only the padding
        CHECK(core.Used() == 16);
    }

    void TestBumpMarkRewindReset()
    {
        BumpCore core(1024);
        (void)core.Alloc(100, 4);
        const BumpCore::Mark mark = core.GetMark();
        (void)core.Alloc(200, 4);
        (void)core.Alloc(300, 4);
        core.Rewind(mark);
        CHECK(core.Used() == 100);
        CHECK(core.Alloc(8, 4) == 100);   // reuses the rewound space

        core.Reset();
        CHECK(core.Used() == 0);
        CHECK(core.Remaining() == 1024);
    }

    // -- GpuBumpAllocator (fake CPU-backed range, no device) ---

    void TestGpuBumpAllocator()
    {
        alignas(256) static byte storage[4096];
        rhi::GpuBumpAllocator alloc;
        alloc.Initialize({
            .cpu  = storage,
            .gpu  = reinterpret_cast<byte*>(uptr(0x100000)),  // fake BDA, never dereferenced
            .size = sizeof(storage),
        });

        const rhi::GpuCpuRange<byte> a = alloc.Alloc(4);
        const rhi::GpuCpuRange<byte> b = alloc.Alloc(16, 256);
        CHECK(a.IsValid() && b.IsValid());
        CHECK(IsAligned(b.cpu, 256));
        CHECK(IsAligned(b.gpu, 256));
        CHECK(b.cpu - storage == b.gpu - reinterpret_cast<byte*>(uptr(0x100000)));  // cpu/gpu stay in lockstep

        struct alignas(16) Constants { float data[12]; };
        const rhi::GpuCpuRange<Constants> c = alloc.Alloc<Constants>(3);
        CHECK(c.IsValid());
        CHECK(c.size == 3 * sizeof(Constants));   // size is bytes, not elements

        CHECK(!alloc.Alloc(8192).IsValid());

        alloc.Reset();
        CHECK(alloc.Used() == 0);
        CHECK(alloc.Alloc(4).cpu == storage);
    }

    // -- SlotCore ---

    void TestSlotCoreAllocateFreeReuse()
    {
        SlotCore<u32> slots;
        slots.Initialize(3);
        CHECK(slots.Allocate() == 0);
        CHECK(slots.Allocate() == 1);
        CHECK(slots.Allocate() == 2);
        CHECK(slots.Allocate() == SlotCore<u32>::kInvalid);   // full
        CHECK(slots.UsedCount() == 3);

        slots.Free(1);
        CHECK(slots.UsedCount() == 2);
        CHECK(slots.Allocate() == 1);   // recycled
        CHECK(slots.Allocate() == SlotCore<u32>::kInvalid);

        slots.Initialize(2);            // re-init drops the old free list
        CHECK(slots.UsedCount() == 0);
        CHECK(slots.Allocate() == 0);
    }

    // -- VirtualHeapCore ---

    void TestVirtualHeapAllocFree()
    {
        rhi::VirtualHeapCore heap;
        CHECK(heap.Initialize(1024));

        const rhi::VirtualHeapAllocation a = heap.Alloc(256, 256);
        const rhi::VirtualHeapAllocation b = heap.Alloc(256, 256);
        CHECK(a.IsValid() && b.IsValid());
        CHECK(a.offset != b.offset);
        CHECK(IsAligned(a.offset, 256) && IsAligned(b.offset, 256));

        CHECK(!heap.Alloc(4096, 16).IsValid());   // too big

        heap.Free(a.handle);
        const rhi::VirtualHeapAllocation c = heap.Alloc(256, 256);
        CHECK(c.IsValid());

        heap.Clear();
        const rhi::VirtualHeapAllocation full = heap.Alloc(1024, 16);   // Clear released everything
        CHECK(full.IsValid() && full.offset == 0);
    }

    void TestVirtualHeapMove()
    {
        rhi::VirtualHeapCore a;
        CHECK(a.Initialize(512));
        (void)a.Alloc(128, 16);

        rhi::VirtualHeapCore b(std::move(a));
        CHECK(!a.IsInitialized());   // source emptied: only b destroys the block
        CHECK(b.IsInitialized());
        CHECK(b.Size() == 512);
        CHECK(b.Alloc(128, 16).IsValid());

        // Move-assign onto an initialized target: its old block is released.
        rhi::VirtualHeapCore c;
        CHECK(c.Initialize(64));
        c = std::move(b);
        CHECK(!b.IsInitialized());
        CHECK(c.Size() == 512);
        // a, b, c destruct here: a crash or VMA assert would mean a double destroy.
    }

    void TestVirtualHeapShutdownWithLiveAllocations()
    {
        rhi::VirtualHeapCore heap;
        CHECK(heap.Initialize(1024));
        const rhi::VirtualHeapAllocation live = heap.Alloc(64, 16);
        CHECK(live.IsValid());

        heap.Shutdown();            // must not trip VMA's leak assert
        CHECK(!heap.IsInitialized());
        heap.Free(live.handle);     // deferred free landing after Shutdown: no-op
        heap.Shutdown();            // idempotent
    }

    // -- LinearAllocator (block mode) ---

    void TestLinearAllocatorConstructsAndAligns()
    {
        alignas(64) static u8 block[1024];
        LinearAllocator alloc;
        alloc.Initialize(block, sizeof(block));

        struct WithDefaults { u32 a = 7; u32 b = 9; };
        WithDefaults* objs = alloc.Alloc<WithDefaults>(4);
        bool all_initialized = true;
        for (u32 i = 0; i < 4; ++i)
            all_initialized &= (objs[i].a == 7 && objs[i].b == 9);
        CHECK(all_initialized);

        (void)alloc.Alloc(1, 1);
        CHECK(IsAligned(alloc.Alloc<u64>(), alignof(u64)));
        CHECK(IsAligned(alloc.Alloc(3), alignof(std::max_align_t)));   // untyped default

        SpanMutable<u32> empty = alloc.AllocSpan<u32>(0);
        CHECK(empty.Size() == 0);

        alloc.Reset();
        CHECK(alloc.GetMark().offset == 0);
        alloc.Shutdown();
    }

    // -- VirtualPageSource ---

    constexpr usize kTestChunk = 64_KB;

    void TestPageSourceRecyclesChunks()
    {
        VirtualPageSource source;
        CHECK(source.Initialize({ .reserve_size = 64_MB, .chunk_size = kTestChunk }));
        const usize chunk = source.ChunkSize();
        CHECK(chunk >= kTestChunk);

        MemoryChunk* a = source.AcquireChunk(100);
        CHECK(a != nullptr && a->size == chunk);
        CHECK(IsAligned(a, LinearAllocator::kMaxAlignment));
        source.ReleaseChunk(a);
        CHECK(source.AcquireChunk(100) == a);           // recycled, no new pages
        CHECK(source.CommittedBytes() == chunk);

        // Oversized: a multi-chunk span, reused first-fit for smaller oversized requests.
        MemoryChunk* big = source.AcquireChunk(3 * chunk);
        CHECK(big->size >= 3 * chunk + kChunkHeaderSize && big->size % chunk == 0);
        const usize committed = source.CommittedBytes();
        source.ReleaseChunk(big);
        CHECK(source.AcquireChunk(2 * chunk) == big);
        CHECK(source.CommittedBytes() == committed);

        source.ReleaseChunk(a);
        source.ReleaseChunk(big);
        source.Shutdown();
        source.ReleaseChunk(a);   // late release after Shutdown: ignored, must not touch unmapped memory
    }

    // -- FrameArena + frame-backed LinearAllocator ---

    void TestFrameAllocatorNewFrameStartsFresh()
    {
        VirtualPageSource source;
        CHECK(source.Initialize({ .reserve_size = 64_MB, .chunk_size = kTestChunk }));
        FrameArena arena;
        arena.Initialize(source);

        LinearAllocator frame;
        frame.Initialize(arena);

        void* a = frame.Alloc(16, 16);
        void* b = frame.Alloc(16, 16);
        CHECK(a != b);

        arena.EndFrame();
        CHECK(arena.CurrentGeneration() == 1);

        // The old chunk was released and (LIFO) handed straight back: a fresh
        // cursor starts at the chunk's first allocation, i.e. where `a` was.
        void* c = frame.Alloc(16, 16);
        CHECK(c == a);

        frame.Shutdown();
        arena.Shutdown();
        source.Shutdown();
    }

    void TestFrameArenaSteadyStateAcrossFrames()
    {
        VirtualPageSource source;
        CHECK(source.Initialize({ .reserve_size = 256_MB, .chunk_size = kTestChunk }));
        FrameArena arena;
        arena.Initialize(source);

        LinearAllocator frame;
        frame.Initialize(arena);

        usize committed_after_warmup = 0;
        bool  ok = true;
        for (u32 f = 0; f < 10000; ++f)
        {
            // Several chunks' worth per frame, including an oversized one.
            for (u32 i = 0; i < 64; ++i)
            {
                u32* p = frame.Alloc<u32>(1024);
                p[0] = f; p[1023] = f;
                ok &= (p[0] == f);
            }
            (void)frame.Alloc(5 * kTestChunk, 64);
            arena.EndFrame();

            if (f == 10)
                committed_after_warmup = source.CommittedBytes();
        }
        CHECK(ok);
        CHECK(committed_after_warmup != 0);
        CHECK(source.CommittedBytes() == committed_after_warmup);   // no growth from churn

        frame.Shutdown();
        arena.Shutdown();
        source.Shutdown();
    }

    void TestFrameAllocatorsAcrossPersistentThreads()
    {
        // Persistent threads, each with its own frame allocator (like Jobs
        // workers), allocate and fill every frame; the main thread verifies
        // everything is intact and distinct before ending the frame. Checks
        // that cursors move to a fresh chunk when the generation changes and
        // that no two threads ever share memory.
        constexpr u32 kThreads = 4, kFrames = 200, kAllocsPerFrame = 300;

        VirtualPageSource source;
        CHECK(source.Initialize({ .reserve_size = 256_MB, .chunk_size = kTestChunk }));
        FrameArena arena;
        arena.Initialize(source);

        struct Record { u32 thread, frame, index, pad; u8 payload[48]; };
        std::vector<Record*> records(kThreads * kAllocsPerFrame, nullptr);

        std::barrier sync(kThreads + 1);
        std::vector<std::thread> threads;
        for (u32 t = 0; t < kThreads; ++t)
        {
            threads.emplace_back([&, t]
            {
                LinearAllocator frame;
                frame.Initialize(arena);
                frame.BindToCurrentThread();
                for (u32 f = 0; f < kFrames; ++f)
                {
                    for (u32 i = 0; i < kAllocsPerFrame; ++i)
                    {
                        Record* r = frame.Alloc<Record>();
                        r->thread = t; r->frame = f; r->index = i;
                        std::memset(r->payload, static_cast<int>((t * 31 + i) & 0xFF), sizeof(r->payload));
                        records[t * kAllocsPerFrame + i] = r;
                    }
                    sync.arrive_and_wait();   // frame work done
                    sync.arrive_and_wait();   // main verified + EndFrame
                }
            });
        }

        bool ok = true;
        for (u32 f = 0; f < kFrames; ++f)
        {
            sync.arrive_and_wait();
            for (u32 t = 0; t < kThreads; ++t)
            {
                for (u32 i = 0; i < kAllocsPerFrame; ++i)
                {
                    const Record* r = records[t * kAllocsPerFrame + i];
                    const u8 expected = static_cast<u8>((t * 31 + i) & 0xFF);
                    ok &= (r->thread == t && r->frame == f && r->index == i);
                    ok &= (r->payload[0] == expected && r->payload[47] == expected);
                }
            }
            arena.EndFrame();
            sync.arrive_and_wait();
        }
        for (std::thread& th : threads)
            th.join();

        CHECK(ok);
        CHECK(arena.CurrentGeneration() == kFrames);

        arena.Shutdown();
        source.Shutdown();
    }

    // -- Page-source-backed LinearAllocator (scratch) ---

    void TestScratchRewindAcrossChunksReusesSpares()
    {
        VirtualPageSource source;
        CHECK(source.Initialize({ .reserve_size = 64_MB, .chunk_size = kTestChunk }));

        LinearAllocator scratch;
        scratch.Initialize(source);

        const LinearAllocator::Mark start = scratch.GetMark();
        for (u32 i = 0; i < 40; ++i)                         // ~400 KB: spans several chunks
            (void)scratch.Alloc(10_KB, 16);
        const usize committed = source.CommittedBytes();

        for (u32 round = 0; round < 100; ++round)
        {
            scratch.Rewind(start);
            for (u32 i = 0; i < 40; ++i)
                (void)scratch.Alloc(10_KB, 16);
        }
        CHECK(source.CommittedBytes() == committed);         // spares reused, nothing new acquired

        // Oversized request in the middle of the chain.
        scratch.Rewind(start);
        (void)scratch.Alloc(100, 8);
        void* big = scratch.Alloc(1_MB, 4096);
        CHECK(IsAligned(big, 4096));
        std::memset(big, 0xAB, 1_MB);

        scratch.Shutdown();
        source.Shutdown();
    }

    void TestScratchScopesNest()
    {
        VirtualPageSource source;
        CHECK(source.Initialize({ .reserve_size = 64_MB, .chunk_size = kTestChunk }));

        ScratchAllocator scratch;
        scratch.Initialize(source);

        void* inner_first = nullptr;
        {
            ScratchScope outer(scratch);
            u32* keep = outer.Alloc<u32>(4);
            keep[0] = 42;
            {
                ScratchScope inner(scratch);
                inner_first = inner.Alloc(256, 16);
                for (u32 i = 0; i < 20; ++i)                  // push the inner scope across chunks
                    (void)inner.Alloc(8_KB, 16);
            }
            CHECK(keep[0] == 42);                             // outer data untouched by inner rewind
            CHECK(outer.Alloc(256, 16) == inner_first);       // inner space is reused
        }
        CHECK(scratch.GetMark().chunk == nullptr && scratch.GetMark().offset == 0);

        scratch.Shutdown();
        source.Shutdown();
    }

    // -- Memory:: thread-local API from Jobs workers ---

    void TestMemoryApiFromJobWorkers()
    {
        // Workers never call any per-thread init: Memory::GetFrameAlloc()
        // must just work on them (Phase 1 finding: it used to assert).
        constexpr u32 kFrames = 50, kJobs = 256;

        Memory::Initialize();
        Jobs::Initialize(4);

        struct Payload { u32 job, frame; u64 check; };
        std::vector<Payload*> results(kJobs, nullptr);
        bool ok = true;

        for (u32 f = 0; f < kFrames; ++f)
        {
            Jobs::Barrier done;
            Payload** out = results.data();
            const auto job = [out, f](const Jobs::DispatchId& id)
            {
                Payload* p = Memory::FrameNew<Payload>(id.global_index, f, u64(id.global_index) * 2654435761u + f);
                {
                    ScratchScope scratch = Memory::Scratch();
                    SpanMutable<u32> tmp = scratch.AllocSpan<u32>(64);
                    tmp[63] = id.global_index;
                    (void)tmp;
                }
                out[id.global_index] = p;
            };
            Jobs::Dispatch(job, kJobs, 8, done);
            Jobs::Wait(done);

            for (u32 j = 0; j < kJobs; ++j)
            {
                const Payload* p = results[j];
                ok &= (p != nullptr && p->job == j && p->frame == f && p->check == u64(j) * 2654435761u + f);
            }

            SpanMutable<u64> zeroed = Memory::FrameArray<u64>(32);
            for (u64 v : zeroed)
                ok &= (v == 0);

            Memory::EndFrame();
        }
        CHECK(ok);

        Jobs::Shutdown();
        Memory::Shutdown();
    }
}

int main()
{
    Run("BumpAlignsOffsetNotSize",           TestBumpAlignsOffsetNotSize);
    Run("BumpExactFitAndOverflow",           TestBumpExactFitAndOverflow);
    Run("BumpPaddingOverflow",               TestBumpPaddingOverflow);
    Run("BumpHugeSizeDoesNotWrap",           TestBumpHugeSizeDoesNotWrap);
    Run("BumpZeroSize",                      TestBumpZeroSize);
    Run("BumpMarkRewindReset",               TestBumpMarkRewindReset);
    Run("GpuBumpAllocator",                  TestGpuBumpAllocator);
    Run("SlotCoreAllocateFreeReuse",         TestSlotCoreAllocateFreeReuse);
    Run("VirtualHeapAllocFree",              TestVirtualHeapAllocFree);
    Run("VirtualHeapMove",                   TestVirtualHeapMove);
    Run("VirtualHeapShutdownWithLiveAllocs", TestVirtualHeapShutdownWithLiveAllocations);
    Run("LinearAllocatorConstructsAndAligns", TestLinearAllocatorConstructsAndAligns);
    Run("PageSourceRecyclesChunks",          TestPageSourceRecyclesChunks);
    Run("FrameAllocatorNewFrameStartsFresh", TestFrameAllocatorNewFrameStartsFresh);
    Run("FrameArenaSteadyStateAcrossFrames", TestFrameArenaSteadyStateAcrossFrames);
    Run("FrameAllocatorsAcrossThreads",      TestFrameAllocatorsAcrossPersistentThreads);
    Run("ScratchRewindReusesSpares",         TestScratchRewindAcrossChunksReusesSpares);
    Run("ScratchScopesNest",                 TestScratchScopesNest);

    Log::Initialize();   // Memory::Initialize logs
    Run("MemoryApiFromJobWorkers",           TestMemoryApiFromJobWorkers);
    Log::Shutdown();

    if (g_failures == 0)
        std::printf("\nALL PASSED (0 failed checks)\n");
    else
        std::printf("\n%d CHECK(S) FAILED\n", g_failures);

    return g_failures == 0 ? 0 : 1;
}
