#pragma once

#include <skypch.h>
#include "core/filesystem.h"
#include "asset_management/asset.h"
#include "asset_management/asset_manager.h"
#include "graphics/vulkan/vk_types.h"
#include <any>

namespace sky
{
class AssetBrowserPopup
{
  public:
    struct AssetBrowserPopupContext
    {
        std::string action = "Save";
        AssetType assetType;
        std::string metadata;
    };

  public:
    AssetBrowserPopup();
    void init();
    void reset();
	void render();
    void showFileBrowserPopup() { m_showFileBrowserModal = true; }
    void setContext(AssetBrowserPopupContext ctx) { m_context = ctx; }

    inline static std::optional<fs::path> s_recentlySavedFile;
    inline static std::optional<fs::path> s_defaultMaterialSavedFile;
    inline static std::optional<fs::path> s_fromMaterialSavedFile;

  private:
	struct FileTreeNode
	{
		std::map<std::string, FileTreeNode> children;
	};
	FileTreeNode m_fileTree;
	FileTreeNode *currentNode = &m_fileTree;
	std::vector<FileTreeNode *> m_nodeStack; // Stack to keep track of visited nodes

  private:
    void fileBrowserPopup();
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
    bool m_showFileBrowserModal = false;

    std::unordered_map<std::string, ImageID> m_icons;

    AssetBrowserPopupContext m_context;
};
}
