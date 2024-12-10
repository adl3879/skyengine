#pragma once

#include <skypch.h>
#include "scene/entity.h"

namespace sky
{
class SceneHierarchyPanel
{
  public:
    void render();
    void setContext(Ref<Scene> ctx) { m_context = ctx; }
    
  private:
    void drawEntityNode(Entity entity, const char *query = "");
    Entity createEntityPopup();
	bool matchesSearchRecursively(Entity &entity, const char *query);
  
  private:
    Ref<Scene> m_context;
};
}