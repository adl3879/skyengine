#pragma once

#include <core/layer.h>
#include <skypch.h>

#include "debug_panels/environment_panel.h"
#include "graphics/vulkan/vk_device.h"
#include "renderer/scene_renderer.h"
#include "scene/scene.h"

#include "debug_panels/scene_hierarchy_panel.h"
#include "debug_panels/project_manager_panel.h"
#include "debug_panels/inspector_panel.h"
#include "debug_panels/asset_browser_panel.h"
#include "debug_panels/asset_browser_popup.h"
#include "debug_panels/logs_panel.h"
#include "debug_panels/titlebar_panel.h"
#include "debug_panels/viewport_panel.h"

namespace sky
{
class EditorLayer : public Layer
{
  public:
    EditorLayer();
    ~EditorLayer() override = default;

    void onAttach() override;
    void onDetach() override;
    void onUpdate(float dt) override;
    void onEvent(Event &e) override;
    void onFixedUpdate(float dt) override;
    void onImGuiRender() override;

  private:
    void setPanelContexts();
    void registerEditorEvents();
    void reset();

    bool onKeyPressed(KeyPressedEvent &e);

  private:
    Ref<SceneRenderer> m_renderer;

    SceneHierarchyPanel m_sceneHierarchyPanel;
    ProjectManagerPanel m_projectManagerPanel;
    InspectorPanel      m_inspectorPanel;
    AssetBrowserPanel   m_assetBrowserPanel;
    AssetBrowserPopup   m_assetBrowserPopup;
    LogsPanel           m_logPanel;
    TitlebarPanel       m_titlebarPanel;
    ViewportPanel       m_viewportPanel;
    EnvironmentPanel    m_environmentPanel;

    Ref<Scene> m_activeScene;
}; 
} // namespace sky
