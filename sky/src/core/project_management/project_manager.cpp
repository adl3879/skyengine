#include "project_manager.h"

#include <fstream>
#include <yaml-cpp/yaml.h>
#include "core/helpers/date_fns.h"

namespace sky
{
void ProjectManager::createNewProject(ProjectConfig config) 
{
    // create folders
    fs::create_directory(config.getProjectFilePath());
    fs::create_directory(config.getProjectFilePath() / config.assetPath);

    serialize(config);

    // add to projects list and serialize
    auto currentProject = ProjectInfo{
        .projectName = config.projectName,
        .projectPath = config.projectPath,
        .projectConfigPath = config.getProjectConfigFilePath(),
        .lastOpened = helper::getCurrentDate(),
    };
    deserializeProjectsList();
    m_projectsList.push_back(currentProject);
    serializeProjectsList();
}

void ProjectManager::loadProject(const fs::path &path)
{
    deserialize(path);
    m_isProjectOpen = true;
}

void ProjectManager::saveProject() 
{
    serialize(m_config);
}

std::vector<ProjectManager::ProjectInfo> ProjectManager::getProjectsList()
{
    deserializeProjectsList();
    return m_projectsList;
}

void ProjectManager::serialize(ProjectConfig config) 
{
    YAML::Emitter out;
    {
        out << YAML::BeginMap;
        out << YAML::Key << "projectName" << YAML::Value << config.projectName;
        out << YAML::Key << "projectPath" << YAML::Value << config.projectPath.string();
        out << YAML::Key << "version" << YAML::Value << config.version;
        out << YAML::Key << "createdDate" << YAML::Value << config.createdDate;
        out << YAML::Key << "lastModifiedDate" << YAML::Value << config.lastModifiedDate;
        out << YAML::Key << "assetPath" << YAML::Value << config.assetPath.string();
        out << YAML::Key << "startScene" << YAML::Value << config.startScene.string();
        out << YAML::EndMap;
    }

    const auto path = config.getProjectConfigFilePath();
    std::ofstream fout(path);
    fout << out.c_str();
}

void ProjectManager::deserialize(const fs::path &path)
{
    YAML::Node data;
    try
    {
        data = YAML::LoadFile(path.string());

        m_config.projectName = data["projectName"].as<std::string>();
        m_config.projectPath = data["projectPath"].as<std::string>();
        m_config.version = data["version"].as<std::string>();
        m_config.createdDate = data["createdDate"].as<std::string>();
        m_config.lastModifiedDate = data["lastModifiedDate"].as<std::string>();
        m_config.assetPath = data["assetPath"].as<std::string>();
        m_config.startScene = data["startScene"].as<std::string>();
    }
    catch (YAML::ParserException e)
    {
        SKY_CORE_ERROR("Failed to load project file '{0}'\n     {1}", path.string(), e.what());
        return;
    }
}

void ProjectManager::serializeProjectsList()
{
    YAML::Emitter out;
    out << YAML::BeginMap;
    {
        out << YAML::Key << "projects" << YAML::Value << YAML::BeginSeq;
        for (const auto &project : m_projectsList)
        {
            out << YAML::BeginMap;
            out << YAML::Key << "projectName" << YAML::Value << project.projectName;
            out << YAML::Key << "projectPath" << YAML::Value << project.projectPath.string();
            out << YAML::Key << "projectConfigPath" << YAML::Value << project.projectConfigPath.string();
            out << YAML::Key << "lastOpened" << YAML::Value << project.lastOpened;
            out << YAML::EndMap;
        }
        out << YAML::EndSeq;
    }
    out << YAML::EndMap;

    const auto path = fs::current_path() / "res/projects.yaml";
    std::ofstream fout(path);
    fout << out.c_str();
}

void ProjectManager::deserializeProjectsList()
{
    const auto path = fs::current_path() / "res/projects.yaml";
    if (!fs::exists(path)) return;
    
    m_projectsList.clear();

    YAML::Node data;
    try
    {
        data = YAML::LoadFile(path.string());
        const auto projects = data["projects"];
        if (projects)
        {
            for (const auto &project : projects)
            {
                ProjectInfo info;
                info.projectName = project["projectName"].as<std::string>();
                info.projectPath = project["projectPath"].as<std::string>();
                info.projectConfigPath = project["projectConfigPath"].as<std::string>();
                info.lastOpened = project["lastOpened"].as<std::string>();
                info.isValid = isProjectValid(info);

                m_projectsList.push_back(info);
            }
        }
    }
    catch (YAML::ParserException e)
    {
        SKY_CORE_ERROR("Failed to load project file '{0}'", e.what());
        return;
    }
}

void ProjectManager::removeProjectFromList(ProjectInfo info)
{
    m_projectsList.erase(std::remove_if(m_projectsList.begin(), m_projectsList.end(),
                                        [=](const ProjectInfo &project)
                                        { return project.projectConfigPath == info.projectConfigPath; }),
                            m_projectsList.end());

    serializeProjectsList();

    // if (info.isValid) fs::remove(info.projectPath);
    
}

bool ProjectManager::isProjectValid(ProjectInfo info)
{
    return fs::exists(info.projectConfigPath);
}
} // namespace sky