#include "EditorLayer.h"
#include "PhxEngine/App/Application.h"
#include <PhxEngine/Graphics/RHI/PhxRHI.h>
#include <PhxEngine/Scene/Components.h>
#include <PhxEngine/Scene/SceneLoader.h>
#include <PhxEngine/Scene/SceneWriter.h>

#include "SceneRenderLayer.h"

#include <iostream>
#include <imgui.h>
#include <imgui_internal.h>

using namespace PhxEngine;
using namespace PhxEngine::Scene;

using namespace PhxEngine::RHI;

#define USE_GLTF_SCENE 1
namespace
{
    template<typename T, typename UIFunc>
    static void DrawComponent(std::string const& name, Entity entity, UIFunc uiFunc)
    {
        const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;

        if (!entity.HasComponent<T>())
        {
            return;
        }

        auto& component = entity.GetComponent<T>();
        ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
        float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
        ImGui::Separator();

        bool open = ImGui::TreeNodeEx((void*)typeid(T).hash_code(), treeNodeFlags, name.c_str());
        ImGui::PopStyleVar();

        ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);
        if (ImGui::Button("+", ImVec2{ lineHeight, lineHeight }))
        {
            ImGui::OpenPopup("ComponentSettings");
        }

        bool removeComponent = false;
        if (ImGui::BeginPopup("ComponentSettings"))
        {
            if (ImGui::MenuItem("Remove component"))
            {
                removeComponent = true;
            }

            ImGui::EndPopup();
        }

        if (open)
        {
            uiFunc(component);
            ImGui::TreePop();
        }

        if (removeComponent)
        {
            entity.RemoveComponent<T>();
        }
    }

    static void DrawFloat3Control(const std::string& label, DirectX::XMFLOAT3& values, float resetValue = 0.0f, float columnWidth = 100.0f)
    {
        ImGuiIO& io = ImGui::GetIO();
        auto boldFont = io.Fonts->Fonts[0];

        ImGui::PushID(label.c_str());

        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, columnWidth);
        ImGui::Text(label.c_str());
        ImGui::NextColumn();

        ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

        // float lineHeight = io.fonts->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
        // ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
        ImGui::PushFont(boldFont);
        if (ImGui::Button("X"))
            values.x = resetValue;
        ImGui::PopFont();
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
        ImGui::PopItemWidth();
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
        ImGui::PushFont(boldFont);
        if (ImGui::Button("Y"))
            values.y = resetValue;
        ImGui::PopFont();
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
        ImGui::PopItemWidth();
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
        ImGui::PushFont(boldFont);
        if (ImGui::Button("Z"))
            values.z = resetValue;
        ImGui::PopFont();
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f");
        ImGui::PopItemWidth();

        ImGui::PopStyleVar();

        ImGui::Columns(1);

        ImGui::PopID();
    }

    static void DrawFloat4Control(const std::string& label, DirectX::XMFLOAT4& values, float resetValue = 0.0f, float columnWidth = 100.0f)
    {
        ImGuiIO& io = ImGui::GetIO();
        auto boldFont = io.Fonts->Fonts[0];

        ImGui::PushID(label.c_str());

        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, columnWidth);
        ImGui::Text(label.c_str());
        ImGui::NextColumn();

        ImGui::PushMultiItemsWidths(4, ImGui::CalcItemWidth());
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

        // float lineHeight = io.fonts->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
        // ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
        ImGui::PushFont(boldFont);
        if (ImGui::Button("X"))
            values.x = resetValue;
        ImGui::PopFont();
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
        ImGui::PopItemWidth();
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
        ImGui::PushFont(boldFont);
        if (ImGui::Button("Y"))
            values.y = resetValue;
        ImGui::PopFont();
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
        ImGui::PopItemWidth();
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
        ImGui::PushFont(boldFont);
        if (ImGui::Button("Z"))
            values.z = resetValue;
        ImGui::PopFont();
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f");
        ImGui::PopItemWidth();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
        ImGui::PushFont(boldFont);
        if (ImGui::Button("W"))
            values.w = resetValue;
        ImGui::PopFont();
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::DragFloat("##W", &values.w, 0.1f, 0.0f, 0.0f, "%.2f");
        ImGui::PopItemWidth();

        ImGui::PopStyleVar();

        ImGui::Columns(1);

        ImGui::PopID();
    }
}

void SceneExplorerPanel::OnRenderImGui()
{
    ImGui::Begin("Scene Explorer");

	if (this->m_scene)
	{

        // Draw the entity nodes
        this->m_scene->GetRegistry().each([&](entt::entity entityId)
            {
                Entity entity = { entityId, this->m_scene.get() };
                if (!entity.HasComponent<New::HierarchyComponent>())
                {
                    this->DrawEntityNode(entity);
                }
            });



        if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
        {
            this->m_selectedEntity = {};
        }

		// Right-click on blank space
		if (ImGui::BeginPopupContextWindow(0, 1, false))
		{
            if (ImGui::MenuItem("Create Empty Entity"))
            {
                this->m_scene->CreateEntity("Empty Entity");
            }

			ImGui::EndPopup();
		}
	}

    ImGui::End();

    ImGui::Begin("Entity Properties");

    if (this->m_selectedEntity)
    {
        this->DrawEntityComponents(this->m_selectedEntity);
    }

    ImGui::End();
}

void SceneExplorerPanel::DrawEntityNode(Entity entity)
{
    auto& name = entity.GetComponent<New::NameComponent>().Name;

    ImGuiTreeNodeFlags flags = ((this->m_selectedEntity == entity) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
    flags |= ImGuiTreeNodeFlags_SpanAvailWidth;
    bool opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity, flags, name.c_str());
    if (ImGui::IsItemClicked())
    {
        this->m_selectedEntity = entity;
    }

    bool entityDeleted = false;
    if (ImGui::BeginPopupContextItem())
    {
        if (ImGui::MenuItem("Delete Entity"))
            entityDeleted = true;

        ImGui::EndPopup();
    }

    if (opened)
    {
        auto view = this->m_scene->GetAllEntitiesWith<New::HierarchyComponent>();
        view.each([&](entt::entity entityId)
            {
                // Draw only top level nodes
                if (view.get<New::HierarchyComponent>(entityId).ParentID == (entt::entity)entity)
                {
                    Entity entity = { entityId, this->m_scene.get() };
                    this->DrawEntityNode(entity);
                }
            });
        /*
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
        bool opened = ImGui::TreeNodeEx((void*)9817239, flags, name.c_str());
        if (opened)
        {
            ImGui::TreePop();
        }
        */
        ImGui::TreePop();
    }

    if (entityDeleted)
    {
        this->m_scene->DestroyEntity(entity);
        if (this->m_selectedEntity == entity)
        {
            this->m_selectedEntity = {};
        }
    }
}

void SceneExplorerPanel::DrawEntityComponents(Entity entity)
{
    if (entity.HasComponent<New::NameComponent>())
    {
        auto& name = entity.GetComponent<New::NameComponent>().Name;

        ImGui::Text(name.c_str());
    }

    ImGui::SameLine();
    ImGui::PushItemWidth(-1);

    if (ImGui::Button("Add Component"))
    {
        ImGui::OpenPopup("AddComponent");
    }

    if (ImGui::BeginPopup("AddComponent"))
    {
        ImGui::EndPopup();
    }

    ImGui::PopItemWidth();


    DrawComponent<New::TransformComponent>("Transform", entity, [](auto& component) {
            DrawFloat3Control("Translation", component.LocalTranslation);
            DrawFloat4Control("Rotation", component.LocalRotation);
            DrawFloat3Control("Scale", component.LocalScale, 1.0f);
        });

    DrawComponent<New::StaticMeshComponent>("StaticMeshComponent", entity, [](auto& component) {
        ImGui::Text("TODO: Add Data");
        });

    DrawComponent<New::DirectionalLightComponent>("DirectionalLightComponent", entity, [](auto& component) {
        ImGui::Text("TODO: Add Data");
        });

    DrawComponent<New::OmniLightComponent>("OmniLightComponent", entity, [](auto& component) {
        ImGui::Text("TODO: Add Data");
        });

    DrawComponent<New::SpotLightComponent>("SpotLightComponent", entity, [](auto& component) {
        ImGui::Text("TODO: Add Data");
        });

    DrawComponent<New::SkyLightComponent>("SkyLightComponent", entity, [](auto& component) {
        ImGui::Text("TODO: Add Data");
        });

    DrawComponent<New::CameraComponent>("CameraComponent", entity, [](auto& component) {
            ImGui::Text("TODO: Add Data");
        });

}

// --------------------------------------------------------------


EditorLayer::EditorLayer(std::shared_ptr<SceneRenderLayer> sceneRenderLayer)
    : AppLayer("Editor Layer")
    , m_sceneRenderLayer(sceneRenderLayer)
{
    this->m_scene = std::make_shared<New::Scene>();

#if !USE_GLTF_SCENE
    this->m_scene->CreateEntity("Hello");
    this->m_scene->CreateEntity("World");

    auto sceneWriter = ISceneWriter::Create();
    bool result = sceneWriter->Write("Assets\\Projects\\Sandbox\\Scenes\\TestScene.json", *this->m_scene);

    assert(result);
#else
    std::unique_ptr<New::ISceneLoader> sceneLoader = PhxEngine::Scene::CreateGltfSceneLoader();
    sceneLoader->LoadScene("Assets\\Models\\MaterialScene\\MatScene.gltf", nullptr, *this->m_scene);
#endif
    this->m_sceneExplorerPanel.SetScene(this->m_scene);
};

void EditorLayer::OnRenderImGui()
{
    this->BeginDockspace();

    this->m_sceneExplorerPanel.OnRenderImGui();

    ImGui::Begin("Viewport");
    ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
    this->m_viewportSize = { viewportPanelSize.x, viewportPanelSize.y };
    if (this->m_sceneRenderLayer)
    {
        PhxEngine::RHI::TextureHandle& colourBuffer = this->m_sceneRenderLayer->GetFinalColourBuffer();

        const::TextureDesc& colourBufferDesc = IGraphicsDevice::Ptr->GetTextureDesc(colourBuffer);
        if (this->m_sceneRenderLayer && (uint32_t)this->m_viewportSize.x != colourBufferDesc.Width || (uint32_t)this->m_viewportSize.y != colourBufferDesc.Height)
        {

            // std::cout << "Resizing Window. New = [" << (uint32_t)this->m_viewportSize.x << ", " << (uint32_t)this->m_viewportSize.y << "]";
            // std::cout << "Current = ["<< colourBuffer->GetDesc().Width  << ", " << colourBuffer->GetDesc().Height << std::endl;
            this->m_sceneRenderLayer->ResizeSurface(this->m_viewportSize);
        }

        ImGui::Image(static_cast<void*>(&colourBuffer), ImVec2{ this->m_viewportSize.x, this->m_viewportSize.y });
    }

    ImGui::End();

    this->EndDockSpace();
}

void EditorLayer::BeginDockspace()
{
    // If you strip some features of, this demo is pretty much equivalent to calling DockSpaceOverViewport()!
    // In most cases you should be able to just call DockSpaceOverViewport() and ignore all the code below!
    // In this specific demo, we are not using DockSpaceOverViewport() because:
    // - we allow the host window to be floating/moveable instead of filling the viewport (when opt_fullscreen == false)
    // - we allow the host window to have padding (when opt_padding == true)
    // - we have a local menu bar in the host window (vs. you could use BeginMainMenuBar() + DockSpaceOverViewport() in your code!)
    // TL;DR; this demo is more complicated than what you would normally use.
    // If we removed all the options we are showcasing, this demo would become:
    //     void ShowExampleAppDockSpace()
    //     {
    //         ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());
    //     }
    static bool dockspaceOpen = true;
    static bool opt_fullscreen = true;
    static bool opt_padding = false;
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

    // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
    // because it would be confusing to have two docking targets within each others.
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    if (opt_fullscreen)
    {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    }
    else
    {
        dockspace_flags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
    }

    // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
    // and handle the pass-thru hole, so we ask Begin() to not render a background.
    if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
        window_flags |= ImGuiWindowFlags_NoBackground;

    // Important: note that we proceed even if Begin() returns false (aka window is collapsed).
    // This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
    // all active windows docked into it will lose their parent and become undocked.
    // We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
    // any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
    if (!opt_padding)
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace Demo", &dockspaceOpen, window_flags);
    if (!opt_padding)
        ImGui::PopStyleVar();

    if (opt_fullscreen)
        ImGui::PopStyleVar(2);

    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("Options"))
        {

            if (ImGui::MenuItem("Close", NULL, false, false))
                PhxEngine::LayeredApplication::Ptr->Close();
            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }
}

void EditorLayer::EndDockSpace()
{
    ImGui::End();
}
