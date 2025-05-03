#pragma once

#include <skypch.h>

#include "scene/scene.h"
#include "core/events/key_event.h"

namespace sky
{
class ViewportPanel
{
  public:
    void render();
    void setContext(Ref<Scene> ctx) { m_context = ctx; }
    void onEvent(Event &e);

  private:
    void drawViewport(const char *title, ImageID image);
    void handleViewportDrop();
    void drawControls(const char *icon, const char *tooltip, bool isActive, std::function<void()> action);
    void drawGizmo(const glm::vec2 &size);
    void drawCameraRect();
    bool onKeyPressed(KeyPressedEvent &e);
    const glm::vec3 getRayIntersectionPoint();

  private:
    Ref<Scene> m_context;
    glm::vec2 m_viewportBounds[2];

    bool m_isControlPressed, m_itemIsDraggedOver;
    int m_gizmoType = -1;
};
} // namespace sky