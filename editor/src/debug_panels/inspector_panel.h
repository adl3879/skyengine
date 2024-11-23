#pragma once

#include <skypch.h>
#include "scene/entity.h"

namespace sky
{
class InspectorPanel
{
  public:
	void render();
	void setContext(Entity ent) { m_context = ent; }

  private:
	void drawTransformComponent();
	void drawMeshComponent();

  private:
	Entity m_context;
};
}