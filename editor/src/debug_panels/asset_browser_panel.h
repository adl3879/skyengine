#pragma once

#include <skypch.h>
#include "core/filesystem.h"
#include "asset_management/asset.h"
#include "graphics/vulkan/vk_types.h"

namespace sky
{
class AssetBrowserPanel
{
  public:
    AssetBrowserPanel();
    void init();
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
    void openCreateFilePopup(AssetType type);
    void confirmDeletePopup();

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

	std::string m_searchQuery;
    bool m_renameRequested = false;
    std::filesystem::path m_renamePath;

    bool m_showConfirmDelete = false;
	fs::path m_filePathToDelete;

    std::unordered_map<std::string, ImageID> m_icons;
};
}
