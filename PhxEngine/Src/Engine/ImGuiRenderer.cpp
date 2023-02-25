#include "D:/Users/C.DiPaolo/Development/Phoenix-Engine/Build/PhxEngine/CMakeFiles/PhxEngine.dir/Debug/cmake_pch.hxx"
#include <PhxEngine/Engine/ImGuiRenderer.h>


#include <PhxEngine/Core/Window.h>

#include <imgui.h>
#include "../Graphics/ImGui/imgui_impl_glfw.h"

using namespace PhxEngine;
using namespace PhxEngine::RHI;


PhxEngine::ImGuiRenderer::ImGuiRenderer(RHI::IGraphicsDevice* gfxDevice, Core::IWindow* window)
    : EngineRenderPass()
{
    this->m_imguiContext = ImGui::CreateContext();
    ImGui::SetCurrentContext(this->m_imguiContext);
    
    auto* glfwWindow = static_cast<GLFWwindow*>(window->GetNativeWindow());

    if (!ImGui_ImplGlfw_InitForVulkan(glfwWindow, true))
    {
        throw std::runtime_error("Failed it initalize IMGUI");
        return;
    }

    ImGuiIO& io = ImGui::GetIO();
    io.FontAllowUserScaling = true;
    // Drive this based on configuration
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows

    unsigned char* pixelData = nullptr;
    int width;
    int height;

    io.Fonts->GetTexDataAsRGBA32(&pixelData, &width, &height);

    // Create texture
    RHI::TextureDesc desc = {};
    desc.Dimension = RHI::TextureDimension::Texture2D;
    desc.Width = width;
    desc.Height = height;
    desc.Format = RHI::RHIFormat::RGBA8_UNORM;
    desc.MipLevels = 1;
    desc.DebugName = "IMGUI Font Texture";

    this->m_fontTexture = IGraphicsDevice::GPtr->CreateTexture(desc);
    io.Fonts->SetTexID(static_cast<void*>(&this->m_fontTexture));
    RHI::SubresourceData subResourceData = {};

    // Bytes per pixel * width of the image. Since we are using an RGBA8, there is 4 bytes per pixel.
    subResourceData.rowPitch = width * 4;
    subResourceData.slicePitch = subResourceData.rowPitch * height;
    subResourceData.pData = pixelData;

    CommandListHandle uploadCommandList = IGraphicsDevice::GPtr->CreateCommandList();

    uploadCommandList->Open();
    uploadCommandList->TransitionBarrier(this->m_fontTexture, RHI::ResourceStates::Common, RHI::ResourceStates::CopyDest);
    uploadCommandList->WriteTexture(this->m_fontTexture, 0, 1, &subResourceData);
    uploadCommandList->TransitionBarrier(this->m_fontTexture, RHI::ResourceStates::CopyDest, RHI::ResourceStates::ShaderResource);

    uploadCommandList->Close();

    IGraphicsDevice::GPtr->ExecuteCommandLists({ uploadCommandList.get() }, true);
}

void PhxEngine::ImGuiRenderer::EnableDarkThemeColours()
{

    auto& colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_WindowBg] = ImVec4{ 0.1f, 0.105f, 0.11f, 1.0f };

    // Headers
    colors[ImGuiCol_Header] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
    colors[ImGuiCol_HeaderHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
    colors[ImGuiCol_HeaderActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

    // Buttons
    colors[ImGuiCol_Button] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
    colors[ImGuiCol_ButtonHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
    colors[ImGuiCol_ButtonActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

    // Frame BG
    colors[ImGuiCol_FrameBg] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
    colors[ImGuiCol_FrameBgHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
    colors[ImGuiCol_FrameBgActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

    // Tabs
    colors[ImGuiCol_Tab] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
    colors[ImGuiCol_TabHovered] = ImVec4{ 0.38f, 0.3805f, 0.381f, 1.0f };
    colors[ImGuiCol_TabActive] = ImVec4{ 0.28f, 0.2805f, 0.281f, 1.0f };
    colors[ImGuiCol_TabUnfocused] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };

    // Title
    colors[ImGuiCol_TitleBg] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
    colors[ImGuiCol_TitleBgActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
}
