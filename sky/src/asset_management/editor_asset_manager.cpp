#include "editor_asset_manager.h"

#include <yaml-cpp/yaml.h>

#include "assert.h"
#include "asset_importer.h"
#include "core/project_management/project_manager.h"

namespace sky
{
static std::map<fs::path, AssetType> s_assetExtensionMap = {
    { ".glfw",  AssetType::Mesh },
    { ".fbx",   AssetType::Mesh },
    { ".glb",   AssetType::Mesh },
};

static AssetType getAssetTypeFromFileExtension(const fs::path &extension)
{
    auto it = s_assetExtensionMap.find(extension);
    if (it == s_assetExtensionMap.end())
    {
        SKY_CORE_WARN("Could not find AssetType for {}", extension.string());
        return AssetType::None;
    }

    return it->second;
};

YAML::Emitter &operator<<(YAML::Emitter &out, const std::string_view &v)
{
    out << std::string(v.data(), v.size());
    return out;
}

Ref<Asset> EditorAssetManager::getAsset(AssetHandle handle)
{
    // 1. check if handle is valid
    if (!isAssetHandleValid(handle)) return nullptr;

    // 2. check if asset needs load (and if so, load)
    Ref<Asset> asset;
    if (isAssetLoaded(handle))
    {
        asset = m_loadedAssets.at(handle);
    }
    else
    {
        // Load dependencies first
        auto metadata = getMetadata(handle);
        for (const auto &dependencyHandle : metadata.dependencies) // Assuming metadata contains a list of dependencies
        {
            // Recursively load each dependency (if not loaded)
            if (!isAssetLoaded(dependencyHandle))
            {
                Ref<Asset> dependencyAsset = getAsset(dependencyHandle);
                if (!dependencyAsset)
                {
                    SKY_CORE_ERROR("Failed to load dependency for asset {}!", static_cast<uint64_t>(handle));
                    return nullptr; // Abort if dependency failed to load
                }
            }
        }

        // 3. Load the main asset
        asset = AssetImporter::importAsset(handle, metadata);
        if (!asset)
        {
            // Import failed
            SKY_CORE_ERROR("EditorAssetManager::GetAsset - asset import failed!");
            return nullptr;
        }

        // Store the loaded asset
        m_loadedAssets[handle] = asset;
    }
    // 3. return asset
    return asset;
}

AssetHandle EditorAssetManager::getOrCreateAssetHandle(fs::path path)
{
    for (const auto& [key, value] : m_assetRegistry)
    {
        if (value.filepath == path) return key;
    }

    return UUID::generate();
}

bool EditorAssetManager::isAssetHandleValid(AssetHandle handle) const 
{
    return handle != 0 && m_assetRegistry.find(handle) != m_assetRegistry.end();
}

AssetType EditorAssetManager::getAssetType(AssetHandle handle) const 
{
    if (!isAssetHandleValid(handle)) return AssetType::None;

    return m_assetRegistry.at(handle).type;
}

void EditorAssetManager::importAsset(const fs::path &filepath)
{
    AssetHandle handle = UUID::generate(); 
    auto metadata = AssetMetadata{
        .type = getAssetTypeFromFileExtension(filepath.extension()),
        .handle = handle,
        .filepath = filepath,
    };
    assert(metadata.type != AssetType::None);
    Ref<Asset> asset = AssetImporter::importAsset(handle, metadata);
    if (asset)
    {
        asset->handle = handle;
        m_assetRegistry[handle] = metadata;
        m_loadedAssets[handle] = asset;
        serializeAssetRegistry();
    }
}

AssetMetadata &EditorAssetManager::getMetadata(AssetHandle handle) 
{
    static AssetMetadata s_NullMetadata;
    auto it = m_assetRegistry.find(handle);
    if (it == m_assetRegistry.end()) return s_NullMetadata;

    return it->second;
}

bool EditorAssetManager::isAssetLoaded(AssetHandle handle) 
{
	return getMetadata(handle).isLoaded;
}

const fs::path &EditorAssetManager::getFilePath(AssetHandle handle) 
{
    return getMetadata(handle).filepath;
}

void EditorAssetManager::serializeAssetRegistry() 
{
    auto path = ProjectManager::getConfig().getAssetRegistryPath();

    YAML::Emitter out;
    {
        out << YAML::BeginMap; // Root
        out << YAML::Key << "assetRegistry" << YAML::Value;

        out << YAML::BeginSeq;
        for (const auto &[handle, metadata] : m_assetRegistry)
        {
            out << YAML::BeginMap;
            out << YAML::Key << "handle" << YAML::Value << handle;
            std::string filepathStr = metadata.filepath.generic_string();
            out << YAML::Key << "filepath" << YAML::Value << filepathStr;
            out << YAML::Key << "type" << YAML::Value << assetTypeToString(metadata.type);
            
            out << YAML::Key << "dependencies" << YAML::Value << YAML::BeginSeq; // Start sequence
            for (const auto &dep : metadata.dependencies)
            {
                out << dep; // Output each dependency handle
            }
            out << YAML::EndSeq; // End sequence
            out << YAML::EndMap;
        }
        out << YAML::EndSeq;
        out << YAML::EndMap; // Root
    }

    std::ofstream fout(path);
    fout << out.c_str();
}

bool EditorAssetManager::deserializeAssetRegistry() 
{
    auto path = ProjectManager::getConfig().getAssetRegistryPath();
    YAML::Node data;
    try
    {
        data = YAML::LoadFile(path.string());
    }
    catch (YAML::ParserException e)
    {
        SKY_CORE_ERROR("Failed to load project file '{0}'\n     {1}", path.string(), e.what());
        return false;
    }

    auto rootNode = data["assetRegistry"];
    if (!rootNode) return false;

    for (const auto &node : rootNode)
    {
        AssetHandle handle = node["handle"].as<uint64_t>();
        auto &metadata = m_assetRegistry[handle]; // inserts new metatdata with handle
        metadata.handle = handle;
        metadata.filepath = node["filepath"].as<std::string>();
        metadata.type = assetTypeFromString(node["type"].as<std::string>());
        
        if (node["dependencies"])
        { // Check if dependencies key exists
            for (const auto &depNode : node["dependencies"])
            {
                AssetHandle dependencyHandle = depNode.as<uint64_t>(); // Read each handle
                metadata.dependencies.push_back(dependencyHandle);     // Add to dependencies
            }
        }
    }

    return true;
}
} // namespace sky