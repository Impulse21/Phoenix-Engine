#pragma once

#define PHX_DEFINE_APP(AppClass)                        \
    PHX::IApplication* phx::CreateApplication() {       \
        return new AppClass();                          \
    }