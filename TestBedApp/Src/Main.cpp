#include <PhxEngine/Core/Log.h>

#include <string>

int main()
{
    PhxEngine::LogSystem::Initialize();

    LOG_CORE_FATAL("Fatal");
    LOG_CORE_ERROR("Error!!");
    LOG_CORE_WARN("Warning!!!");
    LOG_CORE_INFO("INFO");
    LOG_CORE_TRACE("TRACE");


    LOG_FATAL("Fatal");
    LOG_ERROR("Error!!");
    LOG_WARN("Warning!!!");
    LOG_INFO("INFO");
    LOG_TRACE("TRACE");

    int value = 100;
    std::string testMsg = "Testing in:";
    LOG_CORE_FATAL("{0} {1}", value, value);

    LOG_CORE_ERROR("{0} {1}", testMsg, value);
    LOG_CORE_WARN("{0} {1}", testMsg, value);
    LOG_CORE_INFO("{0} {1}", testMsg, value);
    LOG_CORE_TRACE("{0} {1}", testMsg, value);


    LOG_FATAL("{0} {1}", testMsg, value);
    LOG_ERROR("{0} {1}", testMsg, value);
    LOG_WARN("{0} {1}", testMsg, value);
    LOG_INFO("{0} {1}", testMsg, value);
    LOG_TRACE("{0} {1}", testMsg, value);

    return 0;
}