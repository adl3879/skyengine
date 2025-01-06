#include "asset_browser_popup.h"

#include <imgui.h>
#include <IconsFontAwesome5.h>
#include <glm/glm.hpp>
#include <stb_image.h>
#include <FileWatch.h>
#include <tracy/Tracy.hpp>

#include "asset_management/asset_manager.h"
#include "core/project_management/project_manager.h"
#include "core/helpers/file_dialogs.h"
#include "core/application.h"
#include "asset_management/texture_importer.h"
#include "core/tasks/task_manager.h"
#include "core/helpers/image.h"
#include "core/helpers/imgui.h"
#include "scene/scene_manager.h"
#include "core/resource/material_serializer.h"
#include "core/events/event_bus.h"
#include "inspector_panel.h"
#include "scene/scene_serializer.h"

namespace sky
{
static char searchStr[128] = "";
static glm::vec2 thumbnailSize, defaultThumbnailSize = {120.0f, 110.0f};
static float dragRatio = 1.0f;

AssetBrowserPopup::AssetBrowserPopup() 
{
    m_icons["back"]=        helper::loadImageFromFile("res/icons/content_browser/BackIcon.png");
    m_icons["forward"]=     helper::loadImageFromFile("res/icons/content_browser/ForwardIcon.png");
    m_icons["directory"]=   helper::loadImageFromFile("res/icons/content_browser/DirectoryIcon.png");
    m_icons["file"]=        helper::loadImageFromFile("res/icons/content_browser/FileIcon.png");
    m_icons["scene"]=       helper::loadImageFromFile("res/icons/content_browser/SceneIcon.png");
    m_icons["model"]=       helper::loadImageFromFile("res/icons/content_browser/3DModelIcon.png");
}

void AssetBrowserPopup::init()
{
    m_baseDirectory = ProjectManager::getConfig().getAssetDirectory();
    m_currentDirectory = m_baseDirectory;
}

void AssetBrowserPopup::reset() 
{
	init();
}

void AssetBrowserPopup::render()
{
    if (ProjectManager::isProjectOpen()) 
    {
        static bool opened = true;
        if (opened) 
        {
            init();
            opened = false;
        }
    }

    if (m_showFileBrowserModal) ImGui::OpenPopup("File Browser");

    fileBrowserPopup();
    updateThumbnails();
}

void AssetBrowserPopup::fileBrowserPopup() 
{
    if (ImGui::BeginPopupModal("File Browser", &m_showFileBrowserModal))
    {
        float panelWidth = ImGui::GetContentRegionAvail().x;
        float dirTreeWidth = panelWidth * 0.17;

        static float padding = 40.0f;
        const float cellSize = thumbnailSize.x + padding;

        thumbnailSize = defaultThumbnailSize * dragRatio;

        int columnCount = static_cast<int>(panelWidth / cellSize);
        if (columnCount < 1) columnCount = 1;

        // Main content section
        ImGui::BeginChild("MainContent", {0, -80}, false); // Leave 30px for the bottom bar
        {
            ImGui::BeginChild("ContentHeader", {0.f, 45.f}, false,
                              ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
            if (ImGui::ImageButton("##bk", m_icons["back"], {28, 24}, {0, 1}, {1, 0}))
            {
                if (m_currentDirectory != m_baseDirectory)
                {
                    m_DirectoryStack.push_back(m_currentDirectory.stem());
                    m_currentDirectory = m_currentDirectory.parent_path();
                }
            }
            ImGui::SameLine();

            if (ImGui::ImageButton("##fwd", m_icons["forward"], {28, 24}, {0, 1}, {1, 0}))
            {
                if (!m_DirectoryStack.empty())
                {
                    m_currentDirectory /= m_DirectoryStack.back();
                    m_DirectoryStack.pop_back();
                }
            }
            ImGui::SameLine();

            // Search bar
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10);
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 4);

            ImGui::PushItemWidth(350);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5, 5));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.0f);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.05f, 0.05f, 0.05f, 0.54f));
            if (ImGui::InputTextWithHint("##Search", ICON_FA_SEARCH "  Search", searchStr, IM_ARRAYSIZE(searchStr)))
            {
                search(searchStr);
            }
            ImGui::PopStyleColor();
            ImGui::PopStyleVar(2);
            ImGui::PopItemWidth();

            ImGui::SameLine();
            ImGui::SetCursorPosY(0);
            auto filepaths =
                std::filesystem::relative(m_currentDirectory, ProjectManager::getConfig().getAssetDirectory()).string();
            auto assetDirName = ProjectManager::getConfig().assetPath.string();
            filepaths = filepaths == "." ? assetDirName : (assetDirName + "\\" + filepaths);
            ImGui::TextColored({0.5f, 0.5f, 0.5f, 1.0f}, "%s", filepaths.c_str());

            ImGui::SameLine();
            ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 150);

            ImGui::PushItemWidth(150);
            ImGui::SliderFloat("##Thumbnail_Size", &dragRatio, 1.0f, 5.0f);
            ImGui::PopItemWidth();

            ImGui::EndChild();

            ImGui::BeginChild("Content", {0.f, 0.f}, false);

            ImGui::Columns(columnCount, nullptr, false);

            if (m_CurrentDirectoryEntries.empty())
            {
                if (fs::exists(m_currentDirectory))
                {
                    for (auto &directoryEntry : std::filesystem::directory_iterator(m_currentDirectory))
                        drawFileAssetBrowser(directoryEntry);
                }
            }
            else
            {
                for (const auto &directoryEntry : m_CurrentDirectoryEntries) drawFileAssetBrowser(directoryEntry);
            }

            ImGui::Columns(1);
            ImGui::EndChild();
        }
        ImGui::EndChild();

        ImGui::Separator();
        ImGui::SetCursorPosX(40);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 6);

        // Bottom input text bar
        ImGui::BeginChild("BottomBar", {0, 80}, false);
        {
			//ImGui::SetKeyboardFocusHere();

            static char buffer[128] = "\0";
            static std::string tempName = "untitled";
			strcpy(buffer, tempName.c_str());
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10)); // Larger padding
            ImGui::InputText("##abp_input", buffer, IM_ARRAYSIZE(buffer));
            ImGui::PopStyleVar();
            ImGui::SameLine();
            if (ImGui::Button(m_context.action.c_str(), {0, 50}))
            {
                if (m_context.action == "Save")
                {
                    switch (m_context.assetType)
                    {
                        case AssetType::Scene: 
                        {
							const auto path = m_currentDirectory / buffer;
                            auto scene = SceneManager::get().getActiveScene();
                            SceneSerializer serializer(scene);
                            serializer.serialize(fs::relative(path, 
                                ProjectManager::getConfig().getAssetDirectory()), scene->handle);
                            break;
                        }
                        case AssetType::Material:
                        {
							if (strlen(buffer) >= 2)
                            {
								const auto path = m_currentDirectory / (std::string(buffer) + ".mat");
								std::ofstream file(path);
								file.close();

								if (m_context.metadata == "default")
                                {
									s_defaultMaterialSavedFile = fs::relative(path, 
										ProjectManager::getConfig().getAssetDirectory());
                                }
                                else
                                {
									s_fromMaterialSavedFile = fs::relative(path, 
										ProjectManager::getConfig().getAssetDirectory());
                                }
                            }
                            break;
                        }
                        default: break;
                    }
                }
                else 
                {
					switch (m_context.assetType)
                    {
                        default: break;
                    }
                }
                m_showFileBrowserModal = false;
				ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", {0, 50}))
            {
                m_showFileBrowserModal = false;
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndChild();

        ImGui::EndPopup();
    }
}

void AssetBrowserPopup::search(const std::string &query)
{
    m_CurrentDirectoryEntries.clear();

    if (query.empty()) return;

    // convert query to lowercase
    std::string lowerQuery = query;
    std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    for (auto &p : std::filesystem::recursive_directory_iterator(m_baseDirectory))
    {
        auto fileName = p.path().filename().string();
        std::transform(fileName.begin(), fileName.end(), fileName.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        if (fileName.find(lowerQuery) != std::string::npos)
        {
            m_CurrentDirectoryEntries.push_back(p);
        }
    }
}

void AssetBrowserPopup::drawFileAssetBrowser(std::filesystem::directory_entry directoryEntry) 
{
    const auto &path = directoryEntry.path();
    const std::string filenameString = path.filename().string();

    auto relativePath = std::filesystem::relative(path, m_baseDirectory);

    if (path.extension() == ".import") return;
    if (path.extension() == ".bin") return;
    if (m_context.assetType != getAssetTypeFromFileExtension(path.extension()) && !directoryEntry.is_directory()) return;

    ImGui::PushID(filenameString.c_str());
    auto icon = directoryEntry.is_directory() ? m_icons["directory"] : m_icons["file"];
    if (path.extension() == ".scene") icon = m_icons["scene"];

    if (path.extension() == ".png" || path.extension() == ".jpg" || path.extension() == ".jpeg")
    {
        icon = getOrCreateThumbnail(directoryEntry);
        if (icon == NULL_IMAGE_ID) icon = m_icons["file"];
    }

    if (path.extension() == ".mat")
    {
        //icon = ThumbnailManager::Get().GetThumbnail(relativePath);
        if (!icon) icon = m_icons["file"];
    }
    if (path.extension() == ".gltf" || path.extension() == ".fbx" || path.extension() == ".glb")
    {
        icon = m_icons["model"];
    }

    ImGui::BeginGroup();
    ImGui::PushID(filenameString.c_str());

    auto scrPos = ImGui::GetCursorScreenPos();
    auto thumbnailPadding = 20;

    // change button color
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2, 0.2, 0.2, 0.2));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(thumbnailPadding, 5));
    ImGui::ImageButton("##", icon, {thumbnailSize.x, thumbnailSize.y}, {0, 1}, {1, 0});
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);

    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
    {
        m_CurrentDirectoryEntries.clear();
        strcpy(searchStr, "");

        if (directoryEntry.is_directory()) m_currentDirectory /= path.filename();
        else
        {
            auto assetType = getAssetTypeFromFileExtension(path.extension());
            switch (assetType)
            {
                case AssetType::Scene: SceneManager::get().openScene(relativePath); break;
			    default: break;
            }
        }
    }

    std::string fileNameWithoutExtension = filenameString.substr(0, filenameString.find_last_of("."));
    std::string truncatedName = fileNameWithoutExtension;
    if (truncatedName.size() > 10) truncatedName = truncatedName.substr(0, 10) + "...";

    // center
    auto cps = ImGui::GetCursorPosX();
    auto ps = ImGui::GetCursorPosX() +
              (thumbnailSize.x + (thumbnailPadding * 2) - ImGui::CalcTextSize(truncatedName.c_str()).x) / 2;
    ImGui::SetCursorPosX(ps);

	ImGui::TextWrapped("%s", truncatedName.c_str());
	if (ImGui::IsItemHovered())
	{
		ImGui::BeginTooltip();
		ImGui::Text("%s", filenameString.c_str());
		ImGui::EndTooltip();
	}

    ImGui::PopID();
    ImGui::EndGroup();

    ImGui::NextColumn();

    ImGui::PopID();
}

ImageID AssetBrowserPopup::getOrCreateThumbnail(const fs::path &path) 
{
    // 1. Read file timestamp
    // 2. Compare hashed timestamp with existing cached image (in memory first, then from cache file)
    // 3. If equal, return associated thumbnail, otherwise load asset from disk and generate thumbnail
    // 4. If generated new thumbnail, store in cache obviously

    std::filesystem::file_time_type lastWriteTime = std::filesystem::last_write_time(path);
    uint64_t timestamp = std::chrono::duration_cast<std::chrono::seconds>(lastWriteTime.time_since_epoch()).count();

    if (m_cachedImages.find(path) != m_cachedImages.end())
    {
        auto &cachedImage = m_cachedImages.at(path);
        if (cachedImage.timestamp == timestamp) return cachedImage.image;
    }
    m_queue.push({path, timestamp});

    return NULL_IMAGE_ID;
}

void AssetBrowserPopup::updateThumbnails() 
{
    while (!m_queue.empty())
    {
        const auto &thumbnailInfo = m_queue.front();

        if (m_cachedImages.find(thumbnailInfo.assetPath) != m_cachedImages.end())
        {
            auto &cachedImage = m_cachedImages.at(thumbnailInfo.assetPath);
            if (cachedImage.timestamp == thumbnailInfo.timestamp)
            {
                m_queue.pop();
                continue;
            }
        }

        // Check if a task for this asset already exists
        auto existingTask =
            Application::getTaskManager()->getTask<ImageID>("LoadImage_" + thumbnailInfo.assetPath.string());
        if (!existingTask)
        {
            // Create a new task to load the image asynchronously
            auto task =
                CreateRef<Task<ImageID>>("LoadImage_" + thumbnailInfo.assetPath.string(),
                                         [&]() -> ImageID { return helper::loadImageFromFile(thumbnailInfo.assetPath); });
            Application::getTaskManager()->submitTask(task);
        }

        // Get the task's status and result
        auto task = Application::getTaskManager()->getTask<ImageID>("LoadImage_" + thumbnailInfo.assetPath.string());
        if (task->getStatus() == Task<ImageID>::Status::Completed)
        {
            auto result = task->getResult();
            if (result != NULL_IMAGE_ID)
            {
                auto &cachedImage = m_cachedImages[thumbnailInfo.assetPath];
                cachedImage.timestamp = thumbnailInfo.timestamp;
                cachedImage.image = result.value();
            }
            m_queue.pop(); // Task completed; remove it from the queue
        }
        else
        {
            // Task not completed; defer processing this item
            break;
        }
    }
}
}