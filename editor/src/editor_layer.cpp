#include "editor_layer.h"

#include "core/application.h"
#include "scene/scene_manager.h"
#include "scene/entity.h"
#include "scene/components.h"
#include "core/project_management/project_manager.h"

#include <imgui.h>

namespace sky
{
EditorLayer::EditorLayer()
{
    const auto window = Application::getWindow();
    m_activeScene = SceneManager::get().getActiveScene();
    m_activeScene->init();

    m_gfxDevice = CreateRef<gfx::Device>(*window);
    m_renderer = CreateRef<SceneRenderer>(*m_gfxDevice, *m_activeScene);

    auto extent = window->getExtent();
    m_renderer->init({extent.width, extent.height});

    AssimpModelLoader modelLoader("res/models/monkey.glb");
    auto meshId = m_renderer->addMeshToCache(modelLoader.getMeshes()[0]);
    auto monkey = m_activeScene->createEntity("monkey");
    monkey.addComponent<MeshComponent>() = meshId;

    m_renderer->addDrawCommand(
        MeshDrawCommand{
            .meshId = meshId, 
            .modelMatrix = monkey.getComponent<TransformComponent>().getModelMatrix()
        });
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

    auto cmd = m_gfxDevice->beginFrame();
    m_renderer->render(cmd);
    m_gfxDevice->endFrame(cmd, m_renderer->getDrawImage());

    if (m_gfxDevice->needsSwapchainRecreate())
    {
        auto extent = Application::getWindow()->getExtent();
        m_gfxDevice->recreateSwapchain(cmd, extent.width, extent.height);
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

    ImGui::Begin("Viewport");
    auto viewportSize = ImGui::GetContentRegionAvail();
    m_activeScene->setViewportInfo({
        .size = {viewportSize.x, viewportSize.y},
        .isFocus = ImGui::IsWindowFocused(),
    });
    ImGui::Image(m_renderer->getDrawImageId(), viewportSize);
    ImGui::End();

    m_sceneHierarchyPanel.render();
    m_projectManagerPanel.render();

    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
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
        ImGui::EndMenuBar();
    }

    ImGui::End();
}
} // namespace sky