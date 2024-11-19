#include "asset_browser_panel.h"

#include <imgui.h>
#include <IconsFontAwesome5.h>
#include <glm/glm.hpp>

#include "asset_management/asset_manager.h"
#include "core/project_management/project_manager.h"
//#include "core/helpers/file.h"

namespace sky
{
bool openCreateFilePopup = false;

static char searchStr[128] = "";
glm::vec2 thumbnailSize, defaultThumbnailSize = {120.0f, 120.0f};
float dragRatio = 1.0f;

AssetBrowserPanel::AssetBrowserPanel() 
{
    m_baseDirectory = ProjectManager::getConfig().getAssetDirectory();
    m_currentDirectory = m_baseDirectory;
}

void AssetBrowserPanel::render()
{
    m_baseDirectory = ProjectManager::getConfig().getAssetDirectory();

    ImGui::Begin("Content Browser");

    float panelWidth = ImGui::GetContentRegionAvail().x;
    float dirTreeWidth = panelWidth * 0.17;

    static float padding = 80.0f;
    const float cellSize = thumbnailSize.x + padding;

    thumbnailSize = defaultThumbnailSize * dragRatio;

    int columnCount = static_cast<int>(panelWidth / cellSize);
    if (columnCount < 1) columnCount = 1;

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.05f, 0.05f, 0.05f, 0.54f));
    ImGui::BeginChild("Directory Tree", {0.0f, 0.f}, ImGuiChildFlags_ResizeX);
    if (ImGui::TreeNodeEx(ICON_FA_HOME "  Root Directory",
                          ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow))
    {
        if (ImGui::IsItemClicked())
        {
            m_currentDirectory = m_baseDirectory;
            m_nodeStack.clear();
        }
        displayFileHierarchy(m_baseDirectory);
        ImGui::TreePop();
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::SameLine();

    ImGui::BeginChild("Content Region", {0.f, 0.f}, false);
    ImGui::BeginChild("ContentHeader", {0.f, 45.f}, false,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
    if (ImGui::ImageButton("##", m_icons["back"], {28, 24}, {0, 1}, {1, 0}))
    {
        if (m_currentDirectory != m_baseDirectory)
        {
            m_DirectoryStack.push_back(m_currentDirectory.stem());
            m_currentDirectory = m_currentDirectory.parent_path();
        }
    }
    ImGui::SameLine();

    if (ImGui::ImageButton("##", m_icons["forward"], {28, 24}, {0, 1}, {1, 0}))
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
    // search bar
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
    auto filepaths = std::filesystem::relative(m_currentDirectory, ProjectManager::getConfig().getAssetDirectory()).string();
    filepaths = filepaths == "." ? "Assets" : ("Assets\\" + filepaths);
    ImGui::TextColored({0.5f, 0.5f, 0.5f, 1.0f}, "%s", filepaths.c_str());

    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 150);

    ImGui::PushItemWidth(150);
    ImGui::SliderFloat("##Thumbnail_Size", &dragRatio, 1.0f, 5.0f);
    ImGui::PopItemWidth();

    ImGui::EndChild();

    ImGui::BeginChild("Content", {0.f, 0.f}, false);

    ImGui::Columns(columnCount, nullptr, false);

    // Right-click on blank space
    if (ImGui::BeginPopupContextWindow())
    {
        if (ImGui::BeginMenu(ICON_FA_PLUS "  Add Item"))
        {
            if (ImGui::MenuItem(ICON_FA_FOLDER "   New Folder")) openCreateFilePopup(AssetType::Folder);
            if (ImGui::MenuItem(ICON_FA_FILE "   New File")) openCreateFilePopup(AssetType::None);
            ImGui::Separator();
            if (ImGui::MenuItem(ICON_FA_PHOTO_VIDEO "   Scene")) openCreateFilePopup(AssetType::Scene);
            if (ImGui::MenuItem(ICON_FA_PAINT_BRUSH "   Material")) openCreateFilePopup(AssetType::Material);
            if (ImGui::MenuItem(ICON_FA_FILE_CODE "   Shader")) openCreateFilePopup(AssetType::Shader);
            ImGui::EndPopup();
        }
        if (ImGui::BeginMenu(ICON_FA_FILE_IMPORT "   Import"))
        {
            if (ImGui::MenuItem(ICON_FA_IMAGE "   Texture"))
            {
                // load texture file from path and copy to asset directory
                //const auto path = helper::openFile("Image (*.png *.jpg *.jpeg)\0*.png;*.jpg;*.jpeg\0");
                //std::filesystem::copy(path, m_currentDirectory / std::filesystem::path(path).filename());
            }
            if (ImGui::MenuItem(ICON_FA_CUBE "   3D Mesh"))
            {
                //const auto path = helper::openFile("3D Model (*.fbx *.dae *.gltf)\0*.fbx;*.dae;*.gltf\0");
                //MeshImporter::LoadModel(path, m_currentDirectory);
            }
            ImGui::EndPopup();
        }
        if (ImGui::MenuItem(ICON_FA_FOLDER_OPEN "   Open in File Browser"))
        {
            //?
        }
        ImGui::EndPopup();
    }

    if (m_CurrentDirectoryEntries.empty())
    { 
        if (fs::exists(m_currentDirectory))
        {
		    for (auto &directoryEntry : std::filesystem::directory_iterator(m_currentDirectory))
			    drawFileAssetBrowser(directoryEntry);
        }
    }
    else
        for (const auto &directoryEntry : m_CurrentDirectoryEntries) drawFileAssetBrowser(directoryEntry);

    ImGui::Columns(1);
    ImGui::EndChild();

    // TODO: status bar
    ImGui::EndChild();
    ImGui::End();
}

void AssetBrowserPanel::addPathToTree(FileTreeNode &root, const fs::path &path) {}

void AssetBrowserPanel::refreshAssetTree() {}

void AssetBrowserPanel::displayFileHierarchy(const fs::path &directory) 
{
    if (!fs::exists(directory) || !fs::is_directory(directory)) return;

    for (auto &directoryEntry : std::filesystem::directory_iterator(directory))
    {
        const fs::path &entryPath = directoryEntry.path();

        // Get the hash of the node label
        std::size_t labelHash = std::hash<std::string>{}(entryPath.filename().string());
        bool isOpen = false;

        // Check if the entry is a directory
        if (fs::is_directory(entryPath))
        {
            auto path = entryPath.filename().string();
            int flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;

            // check if entryPath is a child of m_CurrentDirectory
            if (m_currentDirectory == entryPath)
            {
                flags |= ImGuiTreeNodeFlags_Selected;
            }

            // check if entryPath is a leaf folder
            bool isLeaf = true;
            for (const auto &p : fs::recursive_directory_iterator(entryPath))
            {
                if (p.is_directory())
                {
                    isLeaf = false;
                    break;
                }
            }
            if (isLeaf) flags |= ImGuiTreeNodeFlags_Leaf;

            const bool treeNodeOpen =
                ImGui::TreeNodeEx(path.c_str(), flags, path.c_str());

            // ImGui::PopStyleColor(3);

            // if treenode is selected
            bool clickedOnArrow =
                (ImGui::GetMousePos().x - ImGui::GetItemRectMin().x) < ImGui::GetTreeNodeToLabelSpacing();
            if (ImGui::IsItemClicked() && !clickedOnArrow) m_currentDirectory = entryPath;

            if (treeNodeOpen)
            {
                isOpen = true;
                displayFileHierarchy(entryPath);
                ImGui::TreePop();
            }
            else
                isOpen = false;
        }
    }
}

void AssetBrowserPanel::openCreateFilePopup(AssetType type) 
{
    if (type == AssetType::Folder)
    {
        std::filesystem::create_directory(m_currentDirectory / "New Folder");
        m_renameRequested = true;
        m_renamePath = m_currentDirectory / "New Folder";
    }
    else
    {
        std::string defaultName = type == AssetType::Scene       ? "Scene.scene"
                                  : type == AssetType::Material  ? "New Material.material"
                                  : type == AssetType::Shader    ? "New Shader.shader"
                                                                 : "New File.txt";

        std::ofstream file(m_currentDirectory / defaultName);
        file.close();
        m_renameRequested = true;
        m_renamePath = m_currentDirectory / defaultName;
	}
}

void AssetBrowserPanel::search(const std::string &query) 
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

void AssetBrowserPanel::drawFileAssetBrowser(std::filesystem::directory_entry directoryEntry) 
{
    const auto &path = directoryEntry.path();
    const std::string filenameString = path.filename().string();

    auto relativePath = std::filesystem::relative(path, m_baseDirectory);

    if (path.stem() == "AssetRegistry") return;
    if (path.extension() == ".import") return;
    if (path.extension() == ".bin") return;

    ImGui::PushID(filenameString.c_str());
    auto icon = directoryEntry.is_directory() ? m_icons["directory"] : m_icons["file"];
    if (path.extension() == ".scene") icon = m_icons["scene"];

    if (path.extension() == ".png" || path.extension() == ".jpg" || path.extension() == ".jpeg")
    {
        //icon = m_ThumbnailCache->GetOrCreateThumbnail(relativePath);
        if (!icon) icon = m_icons["file"];
    }

    if (path.extension() == ".material")
    {
        //icon = ThumbnailManager::Get().GetThumbnail(relativePath);
        if (!icon) icon = m_icons["file"];
    }
    if (path.extension() == ".gltf" || path.extension() == ".fbx" || path.extension() == ".dae")
    {
        icon = m_icons["model"];
    }

    ImGui::BeginGroup();
    ImGui::PushID(filenameString.c_str());

    auto scrPos = ImGui::GetCursorScreenPos();
    auto thumbnailPadding = 20;

    // change button color
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2, 0.2, 0.2, 0.2));

    ImGui::ImageButton("##", icon, {thumbnailSize.x, thumbnailSize.y}, {0, 1}, {1, 0});
    ImGui::PopStyleColor(2);

    if (ImGui::BeginPopupContextItem())
    {
        if (ImGui::MenuItem(ICON_FA_TRASH "   Delete"))
        {
            std::filesystem::remove(path);
        }
        // Rename
        if (ImGui::MenuItem(ICON_FA_PEN "   Rename"))
        {
            m_renameRequested = true;
            m_renamePath = path;
        }
        ImGui::EndPopup();
    }

    if (ImGui::BeginDragDropSource())
    {
        ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", relativePath.string().c_str(),
                                  (strlen(relativePath.string().c_str()) + 1) * sizeof(char *));
        ImGui::Button(relativePath.string().c_str());
        ImGui::EndDragDropSource();
    }

    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
    {
        m_CurrentDirectoryEntries.clear();
        strcpy(searchStr, "");

        if (directoryEntry.is_directory())
            m_currentDirectory /= path.filename();
        else
        {
            if (path.extension() == ".material")
            {
                //auto materialHandle = AssetManager::GetAssetHandleFromPath(relativePath);
                //MaterialEditorPanel::OpenMaterialEditor(materialHandle);
            }
            else if (path.extension() == ".lua" || path.extension() == ".shader")
            {
                //m_Context->InitTextEditor(path);
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
    // ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);

    if (m_renameRequested && m_renamePath == path)
    {
        ImGui::SetKeyboardFocusHere();

        static char name[128] = "\0";
        strcpy(name, fileNameWithoutExtension.c_str());
        ImGui::SetCursorPosX(cps + thumbnailPadding);
        ImGui::PushItemWidth(thumbnailSize.x);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        // text color
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
        // align text to center
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2.0f);
        ImGui::InputText("##rename", name, IM_ARRAYSIZE(name));
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(2);
        ImGui::PopItemWidth();
        if (ImGui::IsKeyPressed(ImGuiKey_Enter, false))
        {
            if (strlen(name) >= 2)
            {
               /* s_data->CurrentRenamePath = path.string();
                m_RenameRequested = false;
                m_RenamePath = "";
                auto extension = path.extension();
                auto newName = extension.empty() ? name : name + extension.string();
                std::filesystem::rename(path, path.parent_path() / newName);*/
            }
        }
    }
    else
    {
        ImGui::TextWrapped("%s", truncatedName.c_str());
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("%s", filenameString.c_str());
            ImGui::EndTooltip();
        }
    }

    ImGui::PopID();
    ImGui::EndGroup();

    // ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 30);

    ImGui::NextColumn();

    ImGui::PopID();
}
} // namespace sky