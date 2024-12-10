#pragma once

#include <skypch.h>

#include "scene/scene.h"

namespace sky
{
class ViewportPanel
{
  public:
	void render();
	void setContext(Ref<Scene> ctx) { m_context = ctx; }

  private:
	void handleViewportDrop();

  private:
	Ref<Scene> m_context;
};
}