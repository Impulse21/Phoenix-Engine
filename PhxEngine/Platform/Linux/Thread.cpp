#include <PhxEngine/Platform/Thread.h>

#include <sys/resource.h>  // setpriority, PRIO_PROCESS
#include <sys/syscall.h>   // SYS_gettid
#include <unistd.h>        // syscall
#include <pthread.h>       // pthread_t, pthread_setschedparam
#include <sched.h>         // SCHED_RR, sched_get_priority_max

using namespace phx::platform;

void Thread::SetThreadName(std::thread& thread, const char* name)
{
    pthread_t handle = thread.native_handle();

    int nameResult = pthread_setname_np(handle, name);
    PHX_ASSERT(nameResult == 0);
}

void Thread::SetThreadAffinity(std::thread& thread, int affinity)
{
    pthread_t handle = thread.native_handle();

    cpu_set_t cpu_set;
    CPU_ZERO(&cpu_set);
    CPU_SET(affinity, &cpu_set);

    int affinityResult = pthread_setaffinity_np(handle, sizeof(cpu_set_t), &cpu_set);
    PHX_ASSERT(affinityResult == 0); // In POSIX, 0 indicates success
}

void Thread::SetThreadPriority(std::thread& thread, Priority prio)
{
    pthread_t handle = thread.native_handle();

    int policy;
    sched_param param;;

    PHX_ASSERT(pthread_getschedparam(handle, &policy, &param) == 0);

    switch (prio)
    {
    case Priority::High:
        policy = SCHED_RR;
        param.sched_priority = sched_get_priority_max(SCHED_RR);
        pthread_setschedparam(handle, policy, &param);
        break;

    case Priority::Low:
        // nice() only affects the calling thread, use setpriority instead
        setpriority(PRIO_PROCESS, syscall(SYS_gettid), 10); // nice +10
        break;
        
    case Priority::Normal:
    default:
        break;
    }
}
