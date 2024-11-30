#pragma once

#include <skypch.h>

#include "asset.h"
#include "core/project_management/project_manager.h"
#include "core/tasks/task_manager.h"
#include "core/application.h"

namespace sky
{
class AssetManager
{
  public:
    template <typename T> 
    static Ref<T> getAsset(AssetHandle handle)
    {
        auto manager = ProjectManager::getEditorAssetManager();
        auto asset = manager->getAsset(handle);
        return std::static_pointer_cast<T>(asset);
    }

	template <typename T> 
    static Ref<Task<T>> loadAssetAsync(AssetHandle handle, std::function<void()> callback = nullptr)
    {
         auto task = CreateRef<Task<T>>(
			"AsyncAssetTask_" + std::to_string(handle),
			[handle]() -> T {
				// Perform the import
				Ref<Asset> asset = ProjectManager::getEditorAssetManager()->getAsset(handle);
				return std::static_pointer_cast<T>(asset);
			});

         if (callback)
         {
             task->setCallback(callback);
         }

         // Submit the task to the TaskManager for asynchronous execution
         Application::getTaskManager()->submitTask(task);

         return task;
    }

    static AssetHandle getOrCreateAssetHandle(fs::path path, AssetType assetType)
    {
        return ProjectManager::getEditorAssetManager()->getOrCreateAssetHandle(path, assetType);
    }

    static AssetMetadata& getMetadata(AssetHandle handle)
    {
        return ProjectManager::getEditorAssetManager()->getMetadata(handle);
    }

    static void addToDependecyList(AssetHandle handle, AssetHandle dependency) 
    {
        auto &deps = AssetManager::getMetadata(handle).dependencies;
        if (std::find(deps.begin(), deps.end(), dependency) == deps.end())
        {
            deps.push_back(dependency); // Add the dependency if it's not already in the list
        }
    }
};
}