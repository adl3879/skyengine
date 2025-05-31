#include "asset_browser_panel.h"

#include <imgui.h>
#include <IconsFontAwesome5.h>
#include <glm/glm.hpp>
#include <stb_image.h>
#include <FileWatch.h>
#include <tracy/Tracy.hpp>

#include "asset_management/asset.h"
#include "asset_management/asset_manager.h"
#include "asset_management/editor_asset_manager.h"
#include "core/log/log.h"
#include "core/project_management/project_manager.h"
#include "core/helpers/file_dialogs.h"
#include "core/application.h"
#include "core/tasks/task_manager.h"
#include "core/helpers/image.h"
#include "core/helpers/imgui.h"
#include "graphics/vulkan/vk_types.h"
#include "imgui_internal.h"
#include "scene/scene_manager.h"
#include "core/events/event_bus.h"
#include "inspector_panel.h"
#include "core/resource/custom_thumbnail.h"
#include "scene/scene_serializer.h"

namespace sky
{
bool openCreateFilePopup = false;

static char searchStr[128] = "";
glm::vec2 thumbnailSize, defaultThumbnailSize = {120.0f, 120.0f};
float dragRatio = 1.0f;

struct ContentBrowserData
{
    std::unique_ptr<filewatch::FileWatch<std::string>> renameWatcher;
    std::string currentRenamePath;
};
ContentBrowserData *s_data = new ContentBrowserData();

AssetBrowserPanel::AssetBrowserPanel() 
{
    m_icons["back"]=        helper::loadImageFromFile("res/icons/content_browser/BackIcon.png");
    m_icons["forward"]=     helper::loadImageFromFile("res/icons/content_browser/ForwardIcon.png");
    m_icons["directory"]=   helper::loadImageFromFile("res/icons/content_browser/DirectoryIcon.png");
    m_icons["file"]=        helper::loadImageFromFile("res/icons/content_browser/FileIcon.png");
    m_icons["scene"]=       helper::loadImageFromFile("res/icons/content_browser/SceneIcon.png");
    m_icons["model"]=       helper::loadImageFromFile("res/icons/content_browser/3DModelIcon.png");
}

void AssetBrowserPanel::init()
{
    m_baseDirectory = ProjectManager::getConfig().getAssetDirectory();
    m_currentDirectory = m_baseDirectory;
}

void AssetBrowserPanel::reset() 
{
	init();
}

void AssetBrowserPanel::render()
{
    ZoneScopedN("Asset browser panel");
    if (ProjectManager::isProjectOpen()) 
    {
        static bool opened = true;
        if (opened) 
        {
            init();
            opened = false;
        }
    }

    // Check if we need to update the directory cache
    std::string currentDirStr = m_currentDirectory.string();
    int64_t currentTime = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    // Update cache if:
    // 1. It's been more than 5 seconds since last update
    // 2. The cache for this directory doesn't exist
    // 3. The cache has been marked as dirty (e.g., after file operations)
    if (m_DirectoryCacheDirty || 
        currentTime - m_LastDirectoryCacheUpdate > 5 || 
        m_DirectoryCache.find(currentDirStr) == m_DirectoryCache.end()) 
    {
        ZoneScopedN("Update directory cache");
        
        m_DirectoryCache[currentDirStr].clear();
        
        if (fs::exists(m_currentDirectory))
        {
            for (auto &directoryEntry : std::filesystem::directory_iterator(m_currentDirectory))
            {
                // Filter out unwanted files here to avoid filtering in the render loop
                const auto &path = directoryEntry.path();
                if (path.stem() == "AssetRegistry") continue;
                if (path.extension() == ".import") continue;
                if (path.extension() == ".bin") continue;
                
                m_DirectoryCache[currentDirStr].push_back(directoryEntry);
            }
        }
        
        m_LastDirectoryCacheUpdate = currentTime;
        m_DirectoryCacheDirty = false;
    }

    ImGui::Begin("Asset Browser   ");

    if (m_showConfirmDelete) ImGui::OpenPopup("Confirm Delete");

    float panelWidth = ImGui::GetContentRegionAvail().x;
    float dirTreeWidth = panelWidth * 0.17;

    static float padding = 60.0f;
    const float cellSize = thumbnailSize.x + padding;

    thumbnailSize = defaultThumbnailSize * dragRatio;

    int columnCount = static_cast<int>(panelWidth / cellSize);
    if (columnCount < 1) columnCount = 1;

    ImGui::BeginChild("Directory Tree", {0.0f, 0.f}, ImGuiChildFlags_ResizeX);
    if (ImGui::TreeNodeEx(ICON_FA_HOME "  Root Directory", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow))
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

    // Add a vertical separator
    ImGui::SameLine();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    
    ImGui::SameLine();

    ImGui::BeginChild("Content Region", {0.f, 0.f}, false);
    ImGui::BeginChild("ContentHeader", {0.f, 45.f}, false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

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

    // Right-click on blank space
	auto originalSpace = ImGui::GetStyle().ItemSpacing.x;
    ImGui::GetStyle().ItemSpacing.y = 12;
    if (ImGui::BeginPopupContextWindow())
    {
		if (ImGui::BeginMenu(ICON_FA_PHOTO_VIDEO "   New Scene")) 
        {
            if (ImGui::MenuItem("2D Scene")) createScene(SceneType::Scene2D);
            if (ImGui::MenuItem("3D Scene")) createScene(SceneType::Scene3D);
            if (ImGui::MenuItem("UI Scene")) createScene(SceneType::SceneUI);
            ImGui::EndMenu();
        }
        if (ImGui::MenuItem(ICON_FA_PAINT_BRUSH "   New Material")) createAssetFile(AssetType::Material);
        if (ImGui::MenuItem(ICON_FA_FILE_CODE "   New Shader")) createAssetFile(AssetType::Shader);
		ImGui::Separator();
		if (ImGui::MenuItem(ICON_FA_FOLDER "   New Folder")) createAssetFile(AssetType::Folder);
        if (ImGui::MenuItem(ICON_FA_FOLDER_OPEN "   Open in File Browser"))
        {
            helper::openFolderInExplorer(ProjectManager::getConfig().getAssetDirectory() / m_currentDirectory);
        }
        ImGui::EndPopup();
    }
    ImGui::GetStyle().ItemSpacing.y = originalSpace;

    if (m_CurrentDirectoryEntries.empty())
    { 
        // Use the cached directory entries instead of accessing the filesystem
        for (const auto &directoryEntry : m_DirectoryCache[currentDirStr])
            drawFileAssetBrowser(directoryEntry);
    }
    else
    {
        for (const auto &directoryEntry : m_CurrentDirectoryEntries) 
            drawFileAssetBrowser(directoryEntry);
    }

    ImGui::Columns(1);
    ImGui::EndChild();

    // TODO: status bar
    ImGui::EndChild();

    confirmDeletePopup();
 
    ImGui::End();
}

void AssetBrowserPanel::handleDroppedFile(const fs::path &path) 
{ 
    importNewAsset(path);
}

void AssetBrowserPanel::importNewAsset(const fs::path &path) 
{
    auto importAsset = [=](const fs::path &path) {
		auto extension = path.extension();
		auto assetType = getAssetTypeFromFileExtension(extension);
		switch (assetType)
		{
			case AssetType::Mesh: loadAsset<Model>(path, assetType); break;
			case AssetType::Texture2D: loadAsset<Texture2D>(path, assetType); break;
			default: break;
		}
	};

    if (fs::is_directory(path))
    {
		fs::copy(path, m_currentDirectory / path.stem());
        // Recursively iterate through the directory
        for (const auto &entry : fs::recursive_directory_iterator(path))
        {
            if (fs::is_regular_file(entry.path()))
            {
                auto file = m_currentDirectory / path.stem() / entry.path().filename();
                importAsset(fs::relative(file, ProjectManager::getConfig().getAssetDirectory()));
            }
        }
    }
    else if (fs::is_regular_file(path))
    {
        fs::path file = m_currentDirectory / path.filename();
        fs::copy(path, file);
        importAsset(fs::relative(file, ProjectManager::getConfig().getAssetDirectory()));
    }
    else
    {
        SKY_CORE_ERROR("Unsupported file type");
    }
}

void AssetBrowserPanel::deleteAsset(const fs::path &path) 
{
	auto remove = [=](const fs::path &path) {
		auto extension = path.extension();
		auto assetType = getAssetTypeFromFileExtension(extension);
		switch (assetType)
		{
			case AssetType::Mesh: removeAsset<Model>(path, assetType); break;
			case AssetType::Texture2D: removeAsset<Texture2D>(path, assetType); break;
			default: break;
		}
	}; 
   
	if (fs::is_directory(path))
	{
		for (const auto &entry : fs::recursive_directory_iterator(path))
		{
			if (fs::is_regular_file(entry.path())) 
                remove(fs::relative(entry.path(), ProjectManager::getConfig().getAssetDirectory()));
		}
	}
    else if (fs::is_regular_file(path)) 
	{
		const auto importDataFile = path.string() + ".import";
        fs::remove(importDataFile);
		remove(fs::relative(path, ProjectManager::getConfig().getAssetDirectory()));
	}
    else SKY_CORE_ERROR("Unsupported file type");
}

void AssetBrowserPanel::addPathToTree(FileTreeNode &root, const fs::path &path) {}

void AssetBrowserPanel::refreshAssetTree() {}

void AssetBrowserPanel::displayFileHierarchy(const fs::path &directory) 
{
    ZoneScopedN("Display file hierarchy");
    if (!fs::exists(directory) || !fs::is_directory(directory)) return;

    // Cache directory structure to avoid repeated filesystem operations
    static std::unordered_map<std::string, bool> leafDirectoryCache;
    static std::unordered_map<std::string, std::vector<fs::directory_entry>> directoryContentsCache;
    static int64_t lastCacheUpdateTime = 0;
    
    // Update cache every few seconds or when explicitly requested
    int64_t currentTime = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    bool updateCache = false;
    std::string dirStr = directory.string();
    
    // Check if we need to update the cache
    if (currentTime - lastCacheUpdateTime > 5 || // Update every 5 seconds
        directoryContentsCache.find(dirStr) == directoryContentsCache.end()) {
        updateCache = true;
        lastCacheUpdateTime = currentTime;
    }
    
    // Update the cache if needed
    if (updateCache) {
        directoryContentsCache[dirStr].clear();
        for (auto &entry : std::filesystem::directory_iterator(directory)) {
            if (fs::is_directory(entry.path())) {
                directoryContentsCache[dirStr].push_back(entry);
                
                // Check if it's a leaf directory (only if not already in cache)
                std::string entryStr = entry.path().string();
                if (leafDirectoryCache.find(entryStr) == leafDirectoryCache.end()) {
                    bool isLeaf = true;
                    for (auto it = fs::directory_iterator(entry.path()); it != fs::directory_iterator(); ++it) {
                        if (fs::is_directory(it->path())) {
                            isLeaf = false;
                            break;
                        }
                    }
                    leafDirectoryCache[entryStr] = isLeaf;
                }
            }
        }
    }
    
    // Display directories from cache
    for (const auto &directoryEntry : directoryContentsCache[dirStr]) {
        const fs::path &entryPath = directoryEntry.path();
        
        // Skip non-directories (already filtered in cache creation)
        auto path = entryPath.filename().string();
        int flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
        
        // Check if entryPath is a child of m_CurrentDirectory
        if (m_currentDirectory == entryPath) {
            flags |= ImGuiTreeNodeFlags_Selected;
        }
        
        // Use cached leaf status
        std::string entryStr = entryPath.string();
        if (leafDirectoryCache[entryStr]) {
            flags |= ImGuiTreeNodeFlags_Leaf;
        }
        
        auto label = std::string(ICON_FA_FOLDER "  ") + path;
        
        ImGui::Spacing();
        const bool treeNodeOpen = ImGui::TreeNodeEx(path.c_str(), flags, label.c_str());
        
        // Handle click
        bool clickedOnArrow = 
            (ImGui::GetMousePos().x - ImGui::GetItemRectMin().x) < ImGui::GetTreeNodeToLabelSpacing();
        if (ImGui::IsItemClicked() && !clickedOnArrow) m_currentDirectory = entryPath;
        
        if (treeNodeOpen) {
            displayFileHierarchy(entryPath);
            ImGui::TreePop();
        }
    }
}

void AssetBrowserPanel::createAssetFile(AssetType type) 
{
    std::filesystem::path newPath;
    std::string defaultName;
    
    if (type == AssetType::Folder)
    {
        defaultName = "New Folder";
        newPath = m_currentDirectory / defaultName;
        std::filesystem::create_directory(newPath);
    }
    else
    {
        switch (type)
        {
            case AssetType::Scene:    defaultName = "New Scene.scene"; break;
            case AssetType::Material: defaultName = "New Material.mat"; break;
            case AssetType::Shader:   defaultName = "New Shader.shader"; break;
            default:                  defaultName = "New File.txt"; break;
        }
        
        newPath = m_currentDirectory / defaultName;
        std::ofstream file(newPath);
        file.close();
    }
    
    // Set up for rename operation
    m_renameRequested = true;
    m_renamePath = newPath;
}

void AssetBrowserPanel::createScene(SceneType type) 
{
    std::string sceneTypePrefix;
    
    switch (type)
    {
        case SceneType::Scene2D: sceneTypePrefix = "2D_"; break;
        case SceneType::Scene3D: sceneTypePrefix = "3D_"; break;
        case SceneType::SceneUI: sceneTypePrefix = "UI_"; break;
        default: sceneTypePrefix = ""; break;
    }
    
    std::string defaultName = "New_" + sceneTypePrefix + "Scene.scene";
    std::filesystem::path newPath = m_currentDirectory / defaultName;
    
    // Create a new scene file with the appropriate scene type
    auto scene = CreateRef<Scene>("Untitled", type);
    
    // Create the empty file first
    std::ofstream file(newPath);
    file.close();
    
    // Then serialize the scene with the correct type
    SceneSerializer serializer(scene);
    serializer.serialize(newPath);
    
    // Set up for rename operation
    m_renameRequested = true;
    m_renamePath = newPath;
}

void AssetBrowserPanel::confirmDeletePopup() 
{
    ImGui::SetNextWindowSize(ImVec2(800, 0), ImGuiCond_Always);
    if (ImGui::BeginPopupModal("Confirm Delete", NULL))
    {
        ImGui::Separator();
        ImGui::Text("Are you sure you want to delete the following file?");
        ImGui::Spacing();
        auto path = ProjectManager::getConfig().getAssetDirectory() / m_filePathToDelete;
        ImGui::TextWrapped("%s", path.string().c_str());
        ImGui::Spacing();

        // Confirmation buttons
        if (helper::imguiButton("Delete", {120, 0}, false, "danger"))
        {
            try
            {
                deleteAsset(path);
                fs::remove_all(path);
            }
            catch (const std::filesystem::filesystem_error &e)
            {
                ImGui::OpenPopup("Error");
            }
            m_showConfirmDelete = false; // Close the confirmation modal
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0)))
        {
            m_showConfirmDelete = false; // Close the confirmation modal
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
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

void AssetBrowserPanel::drawFileAssetBrowser(fs::directory_entry directoryEntry) 
{
    ZoneScopedN("Draw file asset browser");

    const auto &path = directoryEntry.path();
    const std::string filenameString = path.filename().string();

    auto relativePath = std::filesystem::relative(path, m_baseDirectory);

    if (path.stem() == "assetRegistry") return;
    if (path.extension() == ".import")  return;
    if (path.extension() == ".bin")     return;

    // Add a condition to check if the asset is selected
    bool isSelected = (m_selectedAsset == relativePath); 

    ImGui::PushID(filenameString.c_str());

    ImageID icon;
    {
        auto assetType = getAssetTypeFromFileExtension(path.extension());
        icon = CustomThumbnail::get().getOrCreateThumbnail(relativePath);
        
        if (directoryEntry.is_directory()) icon = m_icons["directory"];
        else if (assetType == AssetType::Scene) icon = icon == NULL_IMAGE_ID ? m_icons["scene"] : icon;
    }
    if (icon == NULL_IMAGE_ID) icon = m_icons["file"];

    ImGui::BeginGroup();
    ImGui::PushID(filenameString.c_str());

    auto scrPos = ImGui::GetCursorScreenPos();
    auto thumbnailPadding = 0;

    // change button color
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2, 0.2, 0.2, 0.2));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(thumbnailPadding, thumbnailPadding));
    ImGui::ImageButton("##", icon, {thumbnailSize.x, thumbnailSize.y}, {0, 1}, {1, 0});
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);

    auto originalSpace = ImGui::GetStyle().ItemSpacing.x;
    ImGui::GetStyle().ItemSpacing.y = 12;
    if (ImGui::BeginPopupContextItem())
    {
        if (path.extension() == ".scene" && 
            ImGui::MenuItem(ICON_FA_PHOTO_VIDEO "   Set As Start Scene"))
        {
            ProjectManager::setStartScene(relativePath);
        }
        if (ImGui::MenuItem(ICON_FA_TRASH "   Delete"))
        {
            m_filePathToDelete = relativePath;       // Set the path of the file to delete
            m_showConfirmDelete = true; // Show the confirmation dialog
			ImGui::OpenPopup("Confirm Delete");
        }
        if (ImGui::MenuItem(ICON_FA_PEN "   Rename"))
        {
            m_renameRequested = true;
            m_renamePath = path;
        }
      
        ImGui::EndPopup();
    }
    ImGui::GetStyle().ItemSpacing.y = originalSpace;

    if (ImGui::BeginDragDropSource())
    {
        ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", relativePath.string().c_str(),
            (strlen(relativePath.string().c_str()) + 1) * sizeof(char *));
        ImGui::Button(relativePath.string().c_str());
        ImGui::EndDragDropSource();
    }

    if (ImGui::IsItemClicked()) m_selectedAsset = relativePath;

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
                case AssetType::Material: 
                {
                    const auto handle = AssetManager::getOrCreateAssetHandle(relativePath, AssetType::Material);
					InspectorPanel::MaterialContext ctx;
                    ctx.isCustom = true;
                    ctx.assetHandle = handle;
                    EditorEventBus::get().pushEvent({EditorEventType::OpenMaterialEditor, ctx}); 
                    break;
                }
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

        if (ImGui::IsKeyPressed(ImGuiKey_Escape))
        {
            m_renameRequested = false;
            m_renamePath = "";
        }

        if (ImGui::IsKeyPressed(ImGuiKey_Enter, false))
        {
            if (strlen(name) >= 2)
            {
                s_data->currentRenamePath = path.string();
                m_renameRequested = false;
                m_renamePath = "";
                auto extension = path.extension();
                auto newName = extension.empty() ? name : name + extension.string();
                std::filesystem::rename(path, path.parent_path() / newName);
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

    ImGui::NextColumn();

    ImGui::PopID();
}
} // namespace sky