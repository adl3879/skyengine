#pragma once

#include <skypch.h>
#include "core/filesystem.h"
#include "asset_management/editor_asset_manager.h"

namespace sky
{
class ProjectManager
{
  public:
    struct ProjectConfig
    {
        std::string projectName;
        fs::path    projectPath;
        std::string version;
        std::string createdDate;
        std::string lastModifiedDate;
        fs::path assetPath;
        fs::path startScene;

        ProjectConfig() :
            projectName("untitled"),
            projectPath("C:\\Users\\Toyosi Adekanmbi"),
            version("1.0"),
            assetPath("assets")
        {}
        
        fs::path getProjectFilePath() const { return projectPath / projectName; }
        fs::path getProjectConfigFilePath() const { return getProjectFilePath() / (projectName + ".skyproj"); }
        fs::path getAssetRegistryPath() const { return getProjectFilePath() / assetPath / "assetRegistry.yaml"; }
        fs::path getAssetDirectory() const { return getProjectFilePath() / assetPath; }
        fs::path getImportedCachePath() const { return getProjectFilePath() / ".sky/imported"; }
        fs::path getThumbnailCachePath() const { return getProjectFilePath() / ".sky/thumbnails"; }
    };

    struct ProjectInfo
    {
        std::string projectName;
        fs::path    projectPath;
        fs::path    projectConfigPath;
        std::string lastOpened;
        bool        isValid;

        bool operator==(const ProjectInfo &other) const { return projectPath == other.projectPath; }
    };

  public:
    static void createNewProject(ProjectConfig config);
    static void loadProject(const fs::path &path);
    static void saveProject();
    static void deserializeProjectsList();
    static void removeProjectFromList(ProjectInfo info);
    static void setStartScene(const fs::path &path);

    static ProjectConfig getConfig() { return m_config; }
    static std::vector<ProjectInfo> getProjectsList();
    static bool isProjectOpen() { return m_isProjectOpen; }
    static bool isProjectListEmpty() { return m_projectsList.size() < 0; }
    static std::string getProjectFullName();
    static Ref<EditorAssetManager> getEditorAssetManager();

  private:
    static void serialize(ProjectConfig config);
    static void deserialize(const fs::path &path);
    static void serializeProjectsList();
    
    static bool isProjectValid(ProjectInfo info);
  
  private:
    inline static ProjectConfig m_config;
    inline static fs::path m_projectConfigPath;
    inline static std::vector<ProjectInfo> m_projectsList;
    inline static bool m_isProjectOpen = false;
    inline static Ref<AssetManagerBase> m_assetManager;
};
}