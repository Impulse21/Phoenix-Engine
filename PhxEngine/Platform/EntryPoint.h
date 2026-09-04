#pragma once

namespace phx 
{
    class IApplication;
    
/// @brief Forward declaration of the application entry point. The user must implement this function in their application code. 
    IApplication* CreateApplication();
}

#define PHX_DEFINE_APP(AppClass)                        \
    phx::IApplication* phx::CreateApplication() {       \
        return new AppClass();                          \
    }