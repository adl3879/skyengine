#include "project_manager_panel.h"

#include <imgui.h>
#include <tracy/Tracy.hpp>

#include "graphics/vulkan/vk_imgui_backend.h"
#include "core/project_management/project_manager.h"
#include "core/helpers/file_dialogs.h"
#include "core/helpers/date_fns.h"
#include "core/helpers/imgui.h"

namespace sky
{
void ProjectManagerPanel::render()
{
    ZoneScopedN("Project manager panel");
    if (m_showCreate) ImGui::OpenPopup("Project Manager (New)");
    if (m_showOpen) ImGui::OpenPopup("Project Manager (Open)");
    if (m_confirmRemove) ImGui::OpenPopup("Confirm Remove");
    
    ImVec2 buttonSize(200.0f, 50.0f);

	// create a new project
    ImGui::SetNextWindowSize(ImVec2(800, 390), ImGuiCond_Always);
    if (ImGui::BeginPopupModal("Project Manager (New)", &m_showCreate, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::PushFont(gfx::ImGuiBackend::s_fonts["h2"]);
        ImGui::Text("Configure your new project.");
        ImGui::PopFont();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20);

        // Input field inside the modal (optional)
        static char projectName[128] = "Untitled";
        ImGui::Text("Project name");
        ImGui::SetNextItemWidth(700.0f);
        ImGui::InputText("##pname", projectName, IM_ARRAYSIZE(projectName));
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);
        static auto projectPath = ProjectManager::getConfig().projectPath.string();
        static char location[128] = "";
        strncpy(location, projectPath.c_str(), sizeof(location) - 1);
        location[sizeof(location) - 1] = '\0';
        ImGui::Text("Location");
        ImGui::SetNextItemWidth(700.0f);
        ImGui::InputText("##location", location, IM_ARRAYSIZE(location));
        ImGui::SameLine();
        if (ImGui::Button("..."))
        {
            projectPath = helper::openDirectory();
        }

        // Close button to close the modal
        ImGui::SetCursorPosY(ImGui::GetWindowHeight() - 70);
        if (ImGui::Button("Close", buttonSize))
        {
            ImGui::CloseCurrentPopup();
            m_showCreate = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Create", buttonSize))
        {
            ProjectManager::ProjectConfig config;
            config.projectName = projectName;
            config.projectPath = projectPath;
            config.createdDate = helper::getCurrentDate();
            config.lastModifiedDate = helper::getCurrentDate();
            
            ProjectManager::createNewProject(config);
            strcpy(projectName, "Untitled");

            ImGui::CloseCurrentPopup();
            m_showCreate = false;
        }

        ImGui::EndPopup();
    }

    static int selectedProjectIndex = -1; // Track the selected project index
    auto projectList = ProjectManager::getProjectsList();
    ImGui::SetNextWindowSize(ImVec2(800, 530), ImGuiCond_Always);
    if (ImGui::BeginPopupModal("Project Manager (Open)", &m_showOpen, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::PushFont(gfx::ImGuiBackend::s_fonts["h2"]);
        ImGui::Text("Open existing project.");
        ImGui::PopFont();

        ImGui::BeginChild("ProjectListRegion", ImVec2(0, 300), true);
        // Display each project as a selectable item with a fixed height
        for (int i = 0; i < projectList.size(); ++i)
        {
            auto &project = projectList[i];
            auto alpha = project.isValid ? 1.f : 0.5f; 
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 1.f, alpha));

            if (i > 0 )ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20); // Increase space for item height

            ImGui::PushFont(gfx::ImGuiBackend::s_fonts["h4"]);
            if (ImGui::Selectable(project.projectName.c_str(), selectedProjectIndex == i, 0, ImVec2(0, 100)))
            {
                selectedProjectIndex = i; // Update selected index if clicked
            }
            ImGui::PopFont();


			if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            {
				ProjectManager::loadProject(projectList[selectedProjectIndex].projectConfigPath);
				m_showOpen = false;
				selectedProjectIndex = -1;
				ImGui::CloseCurrentPopup();
            }


            // Display additional project details (e.g., path and last opened date)
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 65);
            ImGui::PushFont(gfx::ImGuiBackend::s_fonts["sm"]);
            ImGui::Text("Location: %s", project.projectPath.string().c_str());
            ImGui::Text("Last Opened: %s", project.lastOpened.c_str());
            ImGui::PopFont();
            ImGui::PopStyleColor();
        }
        ImGui::EndChild();

        const ImVec4 defaultColor(0.0f, 0.5f, 1.0f, 1.0f); // Light blue color
        const ImVec4 hoverColor(0.0f, 0.7f, 1.0f, 1.0f);   // Brighter blue color
        ImVec4 currentColor = defaultColor;
        if (ImGui::IsItemHovered()) currentColor = hoverColor;

        // Change text color to mimic a hyperlink
        ImGui::PushStyleColor(ImGuiCol_Text, currentColor);
        ImGui::Text("Open project file.");
        ImGui::PopStyleColor();

        if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand); // Optional: Change cursor to hand
        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0))
        {
            fs::path file = helper::openFile("Project (*.skyproj)\0*.skyproj\0");
            if (fs::exists(file))
            {
				ProjectManager::loadProject(file);
				m_showOpen = false;
				ImGui::CloseCurrentPopup();
            }
        }

        ImGui::Dummy(ImVec2(0, 20));

        auto select = selectedProjectIndex != -1;
        auto isOpenBtnDisabled = select && projectList[selectedProjectIndex].isValid;
        if (helper::imguiButton("Open", buttonSize, !isOpenBtnDisabled))
        {
            ProjectManager::loadProject(projectList[selectedProjectIndex].projectConfigPath);
            m_showOpen = false;
            selectedProjectIndex = -1;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (helper::imguiButton("Create New", buttonSize))
        {
            m_showCreate = true;
            m_showOpen = false;
        }
        ImGui::SameLine();
        ImGui::Dummy({20, 0});
        ImGui::SameLine();
        if (helper::imguiButton("Remove", buttonSize, !select, "danger"))
        {
            ImGui::OpenPopup("Confirm Remove");
            m_confirmRemove = true;
        }

        ImGui::EndPopup();
    }

    if (ImGui::BeginPopupModal("Confirm Remove", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Are you sure you want to remove this project?");
        ImGui::Separator();

        if (helper::imguiButton("Yes", {120, 0}, false, "danger"))
        {
            ProjectManager::removeProjectFromList(projectList[selectedProjectIndex]);
            ImGui::CloseCurrentPopup();
            m_showOpen = false;
            m_confirmRemove = true;
            selectedProjectIndex = -1;
        }
        ImGui::SameLine();
        if (ImGui::Button("No", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
            m_confirmRemove = false;
            m_showOpen = true;
        }

        ImGui::EndPopup();
    }
}

void ProjectManagerPanel::showOpen() 
{
    ProjectManager::deserializeProjectsList();
    m_showOpen = true;
}
} // namespace sky