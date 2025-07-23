#pragma once

#include <skypch.h>

#include "scene/scene.h"

namespace sky
{
class EnvironmentPanel
{
  public:
	void render();
    void setContext(Ref<Scene> context) { m_context = context; }

    static void toggleShow() { m_isOpen = !m_isOpen; }
    static auto &getIsOpen() { return m_isOpen; }

  private:
   static bool m_isOpen;
   Ref<Scene> m_context;
};
}