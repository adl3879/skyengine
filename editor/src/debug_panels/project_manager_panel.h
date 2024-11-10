#pragma once

#include <skypch.h>

namespace sky
{
class ProjectManagerPanel
{
  public:
    void render();

    void showCreate() { m_showCreate = true; }
    void showOpen();

  private:
    bool m_showCreate = false;
    bool m_showOpen = false;
    bool m_confirmRemove = false;
};
}