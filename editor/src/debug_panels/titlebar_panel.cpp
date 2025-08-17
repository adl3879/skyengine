#include "titlebar_panel.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <tracy/Tracy.hpp>

#include "core/application.h"
#include "core/project_management/project_manager.h"
#include "core/helpers/imgui.h"
#include "environment_panel.h"
#include "core/helpers/image.h"
#include "embed/window_images.embed"
#include "core/events/event_bus.h"
#include "scene/scene_manager.h"

namespace sky
{
TitlebarPanel::TitlebarPanel() : m_titleBarHovered(false) 
{
    m_skyIcon =      helper::loadImageFromFile("res/icons/editor-icon.png");
    m_closeIcon =    helper::loadImageFromData(g_WindowCloseIcon, sizeof(g_WindowCloseIcon));
    m_minimizeIcon = helper::loadImageFromFile("res/icons/minimize-icon.png");
    m_maximizeIcon = helper::loadImageFromData(g_WindowMaximizeIcon, sizeof(g_WindowMaximizeIcon));
    m_restoreIcon =  helper::loadImageFromData(g_WindowRestoreIcon, sizeof(g_WindowRestoreIcon));

    m_playIcon =    helper::loadImageFromFile("res/icons/play.png");
    m_pauseIcon =   helper::loadImageFromFile("res/icons/pause.png");
    m_stopIcon =    helper::loadImageFromFile("res/icons/stop.png");
    m_stepForwardIcon = helper::loadImageFromFile("res/icons/step.png");

    Application::getWindow()->setTitlebarHitTestCallback([&](bool& hit) {
        hit = m_titleBarHovered;
    });
}

void TitlebarPanel::render(float &outTitlebarHeight)
{
    ZoneScopedN("Titlebar panel");
    const bool isMaximized = Application::getWindow()->isWindowMaximized();
    const float titlebarHeight = 90.f;
    float titlebarVerticalOffset = 0.0f;
    const ImVec2 windowPadding = ImGui::GetCurrentWindow()->WindowPadding;

    auto curPos = ImGui::GetCursorPos();
    ImGui::SetCursorPos(ImVec2(windowPadding.x, windowPadding.y + titlebarVerticalOffset));
    const ImVec2 titlebarMin = ImGui::GetCursorScreenPos();
    const ImVec2 titlebarMax = {ImGui::GetCursorScreenPos().x + ImGui::GetWindowWidth() - windowPadding.y * 2.0f,
        ImGui::GetCursorScreenPos().y + titlebarHeight};
    auto *bgDrawList = ImGui::GetBackgroundDrawList();
    auto *fgDrawList = ImGui::GetForegroundDrawList();

    // Logo
    {
        const int logoWidth = 60;
        const int logoHeight = 60;
        const ImVec2 logoOffset(16.0f + windowPadding.x, 5.0f + windowPadding.y + titlebarVerticalOffset);
        const ImVec2 logoRectStart = {ImGui::GetItemRectMin().x + logoOffset.x,
            ImGui::GetItemRectMin().y + logoOffset.y};
        const ImVec2 logoRectMax = {logoRectStart.x + logoWidth, logoRectStart.y + logoHeight};

        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 2.0f + windowPadding.x);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (5.0f + windowPadding.y + titlebarVerticalOffset) / 2.0);
        ImGui::Image(m_skyIcon, ImVec2(logoWidth, logoHeight), ImVec2(0, 1), ImVec2(1, 0));
    }

    const float w = ImGui::GetContentRegionAvail().x;
    const float buttonsAreaWidth = 94;

    // Title bar drag area
    // On Windows we hook into the GLFW win32 window internals
    ImGui::SetCursorPos(ImVec2(windowPadding.x, windowPadding.y + titlebarVerticalOffset)); // Reset cursor pos

    const auto titleBarDragSize = ImVec2(w - buttonsAreaWidth, titlebarHeight);

    if (isMaximized)
    {
        float windowMousePosY = ImGui::GetMousePos().y - ImGui::GetCursorScreenPos().y;
        if (windowMousePosY >= 0.0f && windowMousePosY <= 5.0f)
            m_titleBarHovered = true; // Account for the top-most pixels which don't register
    }

    //auto curPos = ImGui::GetCursorPos();
    bool isOnMenu = false;
    {
        const float logoHorizontalOffset = 5.0f * 2.0f + 62.0f + windowPadding.x;
        ImGui::SetCursorPos(ImVec2(logoHorizontalOffset, 6.0f + titlebarVerticalOffset));
        drawMenuBar();
        if (ImGui::IsItemHovered())
        {
            isOnMenu = true;
        }
    }

    {
        // Centered Window title
        ImVec2 currentCursorPos = ImGui::GetCursorPos();
        std::string title = ProjectManager::getProjectFullName();

        // TODO!
        //if (Engine::GetProject()->IsDirty) title += "*";

        ImVec2 textSize = ImGui::CalcTextSize(title.c_str());
        ImGui::SetCursorPos(
            ImVec2(ImGui::GetWindowWidth() * 0.5f - textSize.x * 0.5f - ImGui::GetStyle().WindowPadding.x / 2.0f,
                   2.0f + windowPadding.y + 6.0f));
        ImGui::Text(title.c_str()); // Draw title
        ImGui::SetCursorPos(currentCursorPos);
    }
    ImGui::SetItemAllowOverlap();

    // Window buttons
    const ImU32 buttonColN = IM_COL32(192, 192, 192, 255);
    const ImU32 buttonColH = IM_COL32(192, 192, 192, 255);
    const ImU32 buttonColP = IM_COL32(192, 192, 192, 255);
    const float buttonWidth = 22.0f;
    const float buttonHeight = 22.0f;
	const auto window = Application::getWindow()->getGLFWwindow();

    // Minimize Button
    ImGui::SameLine();

    const float remaining = ImGui::GetContentRegionAvail().x;
    auto originalSpace = ImGui::GetStyle().ItemSpacing.x;
    ImGui::GetStyle().ItemSpacing.x = 24;
    ImGui::Dummy(ImVec2(remaining - ((buttonWidth + ImGui::GetStyle().ItemSpacing.x) * 3.5), 0));
    ImGui::SameLine();
    {
        if (ImGui::InvisibleButton("Minimize", ImVec2(buttonWidth + 4, buttonHeight + 4)))
        {
            glfwIconifyWindow(window);
        }

        auto rect = ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
        helper::drawButtonImage(m_minimizeIcon, buttonColN, buttonColH, buttonColP);
    }

    ImGui::SameLine();

    // Maximize Button
    {
	    if (ImGui::InvisibleButton("Maximize", ImVec2(buttonWidth, buttonHeight)))
        {
            if (isMaximized)
            {
                glfwRestoreWindow(window);
            }
            else
            {
                glfwMaximizeWindow(window);
            }
        }
        auto rect = ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
        helper::drawButtonImage(isMaximized ? m_restoreIcon : m_maximizeIcon, buttonColN, buttonColH, buttonColP);
    }

    // Close Button
    ImGui::SameLine();
    {
		if (ImGui::InvisibleButton("Close", ImVec2(buttonWidth, buttonHeight)))
        {
            glfwSetWindowShouldClose(window, true);
        }
        helper::drawButtonImage(m_closeIcon, buttonColN, buttonColH, buttonColP);
    }
    ImGui::Dummy({20, 0});
    ImGui::GetStyle().ItemSpacing.x = originalSpace;

    // Second bar with play, stop, pause etc
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
    ImGui::SetCursorPosX(windowPadding.x);
    {
		auto btnSize = ImVec2{30, 30}; 
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 6.f);
		ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2.f - (btnSize.x * 3 / 2));

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.2f, 0.2f, 0.6f));
		if (m_context->isStopped() && ImGui::ImageButton("##play", m_playIcon, btnSize))
		{
            m_context->setSceneState(SceneState::Play);
            SceneManager::get().enterPlayMode();
            ImGui::SetWindowFocus("Game");
		}
		else if (m_context->isPlaying() && ImGui::ImageButton("##stop", m_stopIcon, btnSize))
		{
            m_context->setSceneState(SceneState::Stop);
            SceneManager::get().exitPlayMode();
            ImGui::SetWindowFocus("Scene");
		}
        ImGui::SameLine();
		if (ImGui::ImageButton("##pause", m_pauseIcon, btnSize))
		{
		}
		ImGui::SameLine();
		if (ImGui::ImageButton("##step", m_stepForwardIcon, btnSize))
		{
		}
        ImGui::PopStyleColor(2);
        ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 130.f);
        ImGui::Text("FPS %.2f", Application::getFPS());
	}
    ImGui::PopStyleVar();

    ImGui::SetCursorPos(curPos);
    ImGui::InvisibleButton("##titleBarDragZone", ImVec2(w - buttonsAreaWidth, titlebarHeight / 2));
    m_titleBarHovered = ImGui::IsItemHovered();

    if (isOnMenu) m_titleBarHovered = false;

    ImGui::SetItemAllowOverlap();

    outTitlebarHeight = titlebarHeight;
}

void TitlebarPanel::drawMenuBar() 
{
    const ImRect menuBarRect = {
        ImGui::GetCursorPos(),
        {ImGui::GetContentRegionAvail().x + ImGui::GetCursorScreenPos().x, ImGui::GetFrameHeightWithSpacing()}};
    ImGui::BeginGroup();
       
    if (beginMenubar(menuBarRect))
    {
        if (ImGui::BeginMenu("File"))
        {
			if (ImGui::BeginMenu("Project   "))
			{
				if (ImGui::MenuItem("New Project"))
				{
                    EditorEventBus::get().pushEvent({EditorEventType::NewProject});
				}
				if (ImGui::MenuItem("Open Project"))
				{
                    EditorEventBus::get().pushEvent({EditorEventType::OpenProject});
				}
				ImGui::EndMenu();
			}
            ImGui::Separator();
            if (ImGui::MenuItem("Save Current Scene", "Ctrl+S")) 
            {
				EditorEventBus::get().pushEvent({EditorEventType::SaveCurrentScene});
            }
            if (ImGui::MenuItem("Save Scene As", "Ctrl+Shift+S")) 
            {
				EditorEventBus::get().pushEvent({EditorEventType::SaveSceneAs});
            }
			if (ImGui::MenuItem("Save All Scenes", "Ctrl+Shift+Alt+S")) 
            {
				EditorEventBus::get().pushEvent({EditorEventType::SaveAllScenes});
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit")) 
            {
				EditorEventBus::get().pushEvent({EditorEventType::Exit});
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit")) 
        {
			ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) 
        {
            auto &environmentPanelOpen = EnvironmentPanel::getIsOpen();
            if (ImGui::Checkbox("Environment Panel", &environmentPanelOpen)) 
            {
                EditorEventBus::get().pushEvent({EditorEventType::ToggleEnvironmentPanel});
            }
            
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Tools")) 
        {
			ImGui::EndMenu();
        }
		if (ImGui::BeginMenu("Help")) 
        {
			ImGui::EndMenu();
        }

        endMenubar();
    }

    if (ImGui::IsItemHovered()) m_titleBarHovered = false;

    ImGui::EndGroup();
}

bool TitlebarPanel::beginMenubar(const ImRect &barRect)
{
    ImGuiWindow *window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;

    IM_ASSERT(!window->DC.MenuBarAppending);
    ImGui::BeginGroup(); // Backup position on layer 0 // FIXME: Misleading to use a group for that backup/restore
    ImGui::PushID("##menubar2");

    const ImVec2 padding = window->WindowPadding;
    ImRect result = barRect;
    result.Min.x += 0.0f;
    result.Min.y += padding.y;
    result.Max.x += 0.0f;
    result.Max.y += padding.y;

    // We don't clip with current window clipping rectangle as it is already set to the area below. However we clip with
    // window full rect. We remove 1 worth of rounding to Max.x to that text in long menus and small windows don't tend
    // to display over the lower-right rounded area, which looks particularly glitchy.
    ImRect bar_rect = result; // window->MenuBarRect();
    ImRect clip_rect(IM_ROUND(ImMax(window->Pos.x, bar_rect.Min.x + window->WindowBorderSize + window->Pos.x - 10.0f)),
        IM_ROUND(bar_rect.Min.y + window->WindowBorderSize + window->Pos.y),
        IM_ROUND(ImMax(bar_rect.Min.x + window->Pos.x,
            bar_rect.Max.x - ImMax(window->WindowRounding, window->WindowBorderSize))),
        IM_ROUND(bar_rect.Max.y + window->Pos.y));

    clip_rect.ClipWith(window->OuterRectClipped);
    ImGui::PushClipRect(clip_rect.Min, clip_rect.Max, false);

    // We overwrite CursorMaxPos because BeginGroup sets it to CursorPos (essentially the .EmitItem hack in EndMenuBar()
    // would need something analogous here, maybe a BeginGroupEx() with flags).
    window->DC.CursorPos = window->DC.CursorMaxPos =
        ImVec2(bar_rect.Min.x + window->Pos.x, bar_rect.Min.y + window->Pos.y);
    window->DC.LayoutType = ImGuiLayoutType_Horizontal;
    window->DC.NavLayerCurrent = ImGuiNavLayer_Menu;
    window->DC.MenuBarAppending = true;
    ImGui::AlignTextToFramePadding();
    return true;
}

void TitlebarPanel::endMenubar() 
{
    ImGuiWindow *window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return;
    ImGuiContext &g = *GImGui;

    // Nav: When a move request within one of our child menu failed, capture the request to navigate among our siblings.
    if (ImGui::NavMoveRequestButNoResultYet() && (g.NavMoveDir == ImGuiDir_Left || g.NavMoveDir == ImGuiDir_Right) &&
        (g.NavWindow->Flags & ImGuiWindowFlags_ChildMenu))
    {
        // Try to find out if the request is for one of our child menu
        ImGuiWindow *nav_earliest_child = g.NavWindow;
        while (nav_earliest_child->ParentWindow &&
               (nav_earliest_child->ParentWindow->Flags & ImGuiWindowFlags_ChildMenu))
            nav_earliest_child = nav_earliest_child->ParentWindow;
        if (nav_earliest_child->ParentWindow == window &&
            nav_earliest_child->DC.ParentLayoutType == ImGuiLayoutType_Horizontal &&
            (g.NavMoveFlags & ImGuiNavMoveFlags_Forwarded) == 0)
        {
            // To do so we claim focus back, restore NavId and then process the movement request for yet another frame.
            // This involve a one-frame delay which isn't very problematic in this situation. We could remove it by
            // scoring in advance for multiple window (probably not worth bothering)
            const ImGuiNavLayer layer = ImGuiNavLayer_Menu;
            IM_ASSERT(window->DC.NavLayersActiveMaskNext & (1 << layer)); // Sanity check
            ImGui::FocusWindow(window);
            ImGui::SetNavID(window->NavLastIds[layer], layer, 0, window->NavRectRel[layer]);
            //g.NavDisableHighlight =
            //    true; // Hide highlight for the current frame so we don't see the intermediary selection.
            //g.NavDisableMouseHover = g.NavMousePosDirty = true;
            ImGui::NavMoveRequestForward(g.NavMoveDir, g.NavMoveClipDir, g.NavMoveFlags,
                g.NavMoveScrollFlags); // Repeat
        }
    }

    IM_MSVC_WARNING_SUPPRESS(
        6011); // Static Analysis false positive "warning C6011: Dereferencing NULL pointer 'window'"
    // IM_ASSERT(window->Flags & ImGuiWindowFlags_MenuBar); // NOTE(Yan): Needs to be commented out because Jay
    IM_ASSERT(window->DC.MenuBarAppending);
    ImGui::PopClipRect();
    ImGui::PopID();
    window->DC.MenuBarOffset.x =
        window->DC.CursorPos.x - window->Pos.x; // Save horizontal position so next append can reuse it. This is kinda
                                                // equivalent to a per-layer CursorPos.
    g.GroupStack.back().EmitItem = false;
    ImGui::EndGroup(); // Restore position on layer 0
    window->DC.LayoutType = ImGuiLayoutType_Vertical;
    window->DC.NavLayerCurrent = ImGuiNavLayer_Main;
    window->DC.MenuBarAppending = false;
}
} // namespace sky