#pragma once

#include <skypch.h>
#include "scene/entity.h"

namespace sky
{
class InspectorPanel
{
  public:
	void render();
	void setContext(Ref<Scene> ctx) { m_context = ctx; }

  private:
	void drawTransformComponent();
	void drawMeshComponent();
	void drawDirectionalLightComponent();
	void drawPointLightComponent();
	void drawSpotLightComponent();

  private:
	Ref<Scene> m_context;
	bool m_renameRequested = false;
};
}