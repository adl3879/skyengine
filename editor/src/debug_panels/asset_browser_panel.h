#pragma once

#include <skypch.h>
#include "core/filesystem.h"
#include "asset_management/asset.h"
#include "asset_management/asset_manager.h"
#include "graphics/vulkan/vk_types.h"
#include "scene/scene_manager.h"

namespace sky
{
class AssetBrowserPanel
{
  public:
    AssetBrowserPanel();
    void init();
    void reset();
    void render();
    void handleDroppedFile(const fs::path &path);

  private:
    struct FileTreeNode
    {
        std::map<std::string, FileTreeNode> children;
    };
    FileTreeNode m_fileTree;
    FileTreeNode *currentNode = &m_fileTree;
    std::vector<FileTreeNode *> m_nodeStack; // Stack to keep track of visited nodes

  private:
    void addPathToTree(FileTreeNode &root, const fs::path &path);
    void refreshAssetTree();
    void displayFileHierarchy(const fs::path &directory);
    void createAssetFile(AssetType type);
    void confirmDeletePopup();
    void createScene(SceneType type);

    // import assets
    template <typename T> void loadAsset(const fs::path &path, AssetType assetType)
    {
        auto handle = AssetManager::getOrCreateAssetHandle(path, assetType);
        AssetManager::getAssetAsync<T>(handle);
    }
    template <typename T> void removeAsset(const fs::path &path, AssetType assetType)
    {
        auto handle = AssetManager::getOrCreateAssetHandle(path, assetType);
        AssetManager::removeAsset(handle);
    }
    void importNewAsset(const fs::path &path);
    void deleteAsset(const fs::path &path);

    // Search
    void search(const std::string &query);
    void drawFileAssetBrowser(std::filesystem::directory_entry directoryEntry);

  private:
    ImageID getOrCreateThumbnail(const fs::path &path);
    void updateThumbnails();

    struct ThumbnailImage
    {
        ImageID image;
        uint64_t timestamp;
    };
    struct ThumbnailInfo
    {
        fs::path assetPath;
        uint64_t timestamp;
    };
    std::map<std::filesystem::path, ThumbnailImage> m_cachedImages;
    std::queue<ThumbnailInfo> m_queue;

  private:
    fs::path m_currentDirectory, m_baseDirectory;
    std::vector<fs::path> m_DirectoryStack;
    std::vector<fs::directory_entry> m_CurrentDirectoryEntries;
    fs::path m_selectedAsset;

    std::string m_searchQuery;
    bool m_renameRequested = false;
    std::filesystem::path m_renamePath;

    bool m_showConfirmDelete = false;
    fs::path m_filePathToDelete;

    std::unordered_map<std::string, ImageID> m_icons;
};
} // namespace sky
