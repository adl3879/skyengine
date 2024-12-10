#pragma once

#include <skypch.h>
#include "scene/scene.h"
#include "renderer/texture.h" 

struct ImRect;
namespace sky
{
class TitlebarPanel
{
  public:
	TitlebarPanel();

	void render(float &outTitlebarHeight);
	void setContext(Ref<Scene> ctx) { m_context = ctx; } 

  private:
	void drawMenuBar();
	bool beginMenubar(const ImRect &barRect);
	void endMenubar();

  private:
	Ref<Scene> m_context;

	ImageID m_skyIcon, m_closeIcon, m_maximizeIcon, m_minimizeIcon, m_restoreIcon; 
	ImageID m_playIcon, m_pauseIcon, m_stopIcon, m_stepForwardIcon;
	bool m_titleBarHovered;
};
}