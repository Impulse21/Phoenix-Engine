#pragma once

#define PHX_DEFINE_APP(AppClass)                        \
    phx::IApplication* phx::CreateApplication() {       \
        return new AppClass();                          \
    }