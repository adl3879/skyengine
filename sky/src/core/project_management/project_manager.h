#pragma once

#include <skypch.h>
#include "core/filesystem.h"

namespace sky
{

class ProjectManager
{
  public:
    struct ProjectConfig
    {
        std::string projectName = "untitled";
        fs::path    projectPath = "C:\\Users\\Toyosi Adekanmbi";
        std::string version = "1.0";
        std::string createdDate;
        std::string lastModifiedDate;

        fs::path assetPath = "res/";
        fs::path startScene;
        
        fs::path getProjectFilePath() const { return projectPath / projectName; }
        fs::path getProjectConfigFilePath() const { return getProjectFilePath() / (projectName + ".skyproj"); }
    };

    struct ProjectInfo
    {
        std::string projectName;
        fs::path    projectPath;
        fs::path    projectConfigPath;
        std::string lastOpened;
        bool        isValid;
    };

  public:
    static void createNewProject(ProjectConfig config);
    static void loadProject(const fs::path &path);
    static void saveProject();
    static void deserializeProjectsList();
    static void removeProjectFromList(ProjectInfo info);

    static ProjectConfig getConfig() { return m_config; }
    static std::vector<ProjectInfo> getProjectsList();
    static bool isProjectOpen() { return m_isProjectOpen; }

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
};
}