#include "editor_layer.h"

#include "core/application.h"
#include "core/events/input.h"
#include "core/events/key_codes.h"
#include "core/resource/custom_thumbnail.h"
#include "scene/scene_manager.h"
#include "core/project_management/project_manager.h"
#include "core/events/event_bus.h"

#include <imgui.h>
#include <imgui_internal.h>

namespace sky
{
EditorLayer::EditorLayer()
{
    const auto window = Application::getWindow();
    m_activeScene = SceneManager::get().getActiveScene();
    m_activeScene->init();

    m_renderer = Application::getRenderer();

    auto extent = window->getExtent();
    m_renderer->init({extent.width, extent.height});

    if (!ProjectManager::isProjectOpen())
    {
        if (!ProjectManager::isProjectListEmpty()) m_projectManagerPanel.showOpen();
        else m_projectManagerPanel.showCreate();
    }
    registerEditorEvents();
}

void EditorLayer::onAttach() 
{
    setPanelContexts();
}

void EditorLayer::onDetach() 
{
    m_activeScene->cleanup();
}

void EditorLayer::onUpdate(float dt) 
{
    m_activeScene = SceneManager::get().getActiveScene();
    m_activeScene->update(dt);
    setPanelContexts();

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
    m_viewportPanel.onEvent(e);

    EventDispatcher dispatcher(e);
    dispatcher.dispatch<KeyPressedEvent>(SKY_BIND_EVENT_FN(EditorLayer::onKeyPressed));
}


bool EditorLayer::onKeyPressed(KeyPressedEvent &e)
{
    switch (e.getKeyCode())
    {
        case Key::F:
        {
            if (Input::isKeyPressed(Key::LeftShift)) 
                m_activeScene->getEditorCamera()->toggleFreeLook();
            break;
        }
    }
    return true;
}

void EditorLayer::onFixedUpdate(float dt) {}

void EditorLayer::onImGuiRender()
{
    ZoneScopedN("Editor imgui render");
    static ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_None;

    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDocking;
	const ImGuiViewport *viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	ImGui::SetNextWindowViewport(viewport->ID);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoScrollbar;

    if (dockspaceFlags & ImGuiDockNodeFlags_PassthruCentralNode) windowFlags |= ImGuiWindowFlags_NoBackground;

    auto isMaximized = Application::getWindow()->isWindowMaximized();
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.1f, 1.f));
    ImGui::PushStyleColor(ImGuiCol_MenuBarBg, {0.f, 0.f, 0.f, 0.f});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, isMaximized ? ImVec2(6.0f, 6.0f) : ImVec2(1.0f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 3.0f);
    ImGui::Begin("MyDockSpace", nullptr, windowFlags);
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
    ImGui::PopStyleVar(2);

    {
        float titleBarHeight;
        m_titlebarPanel.render(titleBarHeight);
        ImGui::SetCursorPosY(titleBarHeight);
    }

    // Dockspace
    ImGuiIO &io = ImGui::GetIO();
    ImGuiStyle &style = ImGui::GetStyle();
    float minWinSizeX = style.WindowMinSize.x;
    style.WindowMinSize.x = 370.0f;
    ImGui::DockSpace(ImGui::GetID("MyDockspace"));
    style.WindowMinSize.x = minWinSizeX;

    m_viewportPanel.render();
    m_sceneHierarchyPanel.render();
    m_projectManagerPanel.render();
    m_inspectorPanel.render();
    m_assetBrowserPanel.render();
    m_assetBrowserPopup.render();
    m_logPanel.render();
    m_environmentPanel.render();

    ImGui::End();
}

void EditorLayer::setPanelContexts() 
{
	m_viewportPanel.setContext(m_activeScene);
    m_sceneHierarchyPanel.setContext(m_activeScene);
    m_titlebarPanel.setContext(m_activeScene);
    m_inspectorPanel.setContext(m_activeScene);
}

void EditorLayer::registerEditorEvents() 
{
    auto &eventBus = EditorEventBus::get();

    eventBus.registerHandler(EditorEventType::Exit, [](const EditorEvent &event){
        Application::quit();    
    });
	eventBus.registerHandler(EditorEventType::NewProject, [=](const EditorEvent &event){
        m_projectManagerPanel.showCreate();
    });
	eventBus.registerHandler(EditorEventType::OpenProject, [=](const EditorEvent &event){
        m_projectManagerPanel.showOpen();
    });
    eventBus.registerHandler(EditorEventType::SaveCurrentScene, [=](const EditorEvent &event){
        SceneManager::get().saveActiveScene();
    });
    eventBus.registerHandler(EditorEventType::SaveSceneAs, [=, this](const EditorEvent &event){
        m_assetBrowserPopup.showFileBrowserPopup();
		m_assetBrowserPopup.setContext({
            .action = "Save",
            .assetType = AssetType::Scene,
        });
    });
	eventBus.registerHandler(EditorEventType::Reset, [this](const EditorEvent &event){
        reset();
    });
	eventBus.registerHandler(EditorEventType::OpenMaterialEditor, [=](const EditorEvent &event){
        m_inspectorPanel.openView(InspectorPanelView::MaterialEditor);
        if (event.data.has_value())
        {
            auto ctx = std::any_cast<InspectorPanel::MaterialContext>(event.data);
            m_inspectorPanel.setMaterialContext(ctx);
            CustomThumbnail::get().setMaterialPreviewAssetHandle(ctx.assetHandle);
        }
    });
	eventBus.registerHandler(EditorEventType::CreateNewMaterialFrom, [=](const EditorEvent &event){
        m_assetBrowserPopup.showFileBrowserPopup();
        m_assetBrowserPopup.setContext({
            .action = "Save",
            .assetType = AssetType::Material,
            .metadata = std::any_cast<const char *>(event.data), 
        });
    });
	eventBus.registerHandler(EditorEventType::CreateDefaultMaterial, [=](const EditorEvent &event){
        m_assetBrowserPopup.showFileBrowserPopup();
        m_assetBrowserPopup.setContext({
            .action = "Save",
            .assetType = AssetType::Material,
            .metadata = std::any_cast<const char *>(event.data), 
        });
    });
}

void EditorLayer::reset() 
{
    m_assetBrowserPanel.reset();
    m_logPanel.reset();
}
} // namespace sky