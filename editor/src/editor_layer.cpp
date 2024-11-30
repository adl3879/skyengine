#include "editor_layer.h"

#include "core/application.h"
#include "scene/scene_manager.h"
#include "scene/entity.h"
#include "scene/components.h"
#include "core/project_management/project_manager.h"
#include "core/helpers/imgui.h"
#include "asset_management/asset_manager.h"

#include <imgui.h>

namespace sky
{
EditorLayer::EditorLayer()
{
    const auto window = Application::getWindow();
    m_activeScene = SceneManager::get().getActiveScene();
    m_activeScene->init();

    m_renderer = Application::getRenderer();

    auto extent = window->getExtent();
    m_renderer->init(m_activeScene, {extent.width, extent.height});

    AssimpModelLoader modelLoader("res/models/monkey.glb");
    auto meshId = m_renderer->addMeshToCache(modelLoader.getMeshes()[0].mesh);
    auto monkey = m_activeScene->createEntity("Monkey");
    auto &msh = monkey.addComponent<MeshComponent>();
    msh.meshID = meshId;

    m_renderer->addDrawCommand(
        MeshDrawCommand{
            .meshId = meshId, 
            .modelMatrix = monkey.getComponent<TransformComponent>().getModelMatrix(),
            .isVisible = true
        });

    if (!ProjectManager::isProjectOpen())
    {
        if (!ProjectManager::isProjectListEmpty()) m_projectManagerPanel.showOpen();
        else m_projectManagerPanel.showCreate();
    }
}

void EditorLayer::onAttach() 
{
    m_sceneHierarchyPanel.setContext(m_activeScene);
}

void EditorLayer::onDetach() 
{
    m_activeScene->cleanup();
}

void EditorLayer::onUpdate(float dt) 
{
    m_activeScene->update(dt);
    m_inspectorPanel.setContext(m_sceneHierarchyPanel.getSelectedEntity());

    auto &droppedFiles = Application::getDroppedFiles();
    while (!droppedFiles.empty())
    {
        auto filename = droppedFiles.back();
        droppedFiles.pop_back();
        m_assetBrowserPanel.handleDroppedFile(filename);
    }
}

void EditorLayer::onEvent(Event &e) 
{
    m_activeScene->onEvent(e);
}

void EditorLayer::onFixedUpdate(float dt) {}

void EditorLayer::onImGuiRender()
{
    static bool dockspaceOpen = true;
    static bool optFullscreen = true;
    static bool optPadding = false;
    static ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_None;

    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    if (optFullscreen)
    {
        const ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                        ImGuiWindowFlags_NoMove;
        windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    }
    else
        dockspaceFlags &= ~ImGuiDockNodeFlags_PassthruCentralNode;

    if (dockspaceFlags & ImGuiDockNodeFlags_PassthruCentralNode) windowFlags |= ImGuiWindowFlags_NoBackground;

    if (!optPadding) ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace", &dockspaceOpen, windowFlags);
    if (!optPadding) ImGui::PopStyleVar();

    if (optFullscreen) ImGui::PopStyleVar(2);

    // Submit the DockSpace
    ImGuiIO &io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspaceFlags);
    }

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("Viewport");
    auto viewportSize = ImGui::GetContentRegionAvail();
    m_activeScene->setViewportInfo({
        .size = {viewportSize.x, viewportSize.y},
        .isFocus = ImGui::IsWindowFocused(),
    });
    ImGui::Image(m_renderer->getDrawImageId(), viewportSize);

    if (ImGui::BeginDragDropTarget())
    {
        handleViewportDrop();
        ImGui::EndDragDropTarget(); 
    }
    ImGui::End();
    ImGui::PopStyleVar();

    m_sceneHierarchyPanel.render();
    m_projectManagerPanel.render();
    m_inspectorPanel.render();
    m_assetBrowserPanel.render();
    m_logPanel.render();

    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Quit")) Application::quit();
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Project"))
        {
            if (ImGui::MenuItem("New Project"))
            {
                m_projectManagerPanel.showCreate();
            }
            if (ImGui::MenuItem("Open Project"))
            {
                m_projectManagerPanel.showOpen();
            }
            ImGui::EndMenu();
        }
        
        helper::imguiCenteredText(ProjectManager::getProjectFullName());
        ImGui::EndMenuBar();
    }

    ImGui::End();
}

void EditorLayer::handleViewportDrop() 
{
    if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
    {
        fs::path path = (const char *)payload->Data;
        SKY_CORE_INFO("Dropped file: {}", path.string());
        auto assetType = getAssetTypeFromFileExtension(path.extension());

        switch (assetType)
        {
            case AssetType::Mesh:
            {
                auto handle = AssetManager::getOrCreateAssetHandle(path, assetType);
                auto asset = AssetManager::getAsset<Model>(handle);
				break;
            }
            case AssetType::Scene: break;
            case AssetType::Material: break;
            default: break;
        }
    }
}
} // namespace sky