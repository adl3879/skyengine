#pragma once

#include <skypch.h>

#include "core/filesystem.h"

namespace sky
{
class EnvironmentPanel
{
  public:
	void render();

    static void toggleShow() { m_isOpen = !m_isOpen; }
    static auto &getIsOpen() { return m_isOpen; }

  private:
   static bool m_isOpen;
};
}