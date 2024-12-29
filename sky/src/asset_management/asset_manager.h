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
    static Ref<Task<Ref<T>>> getAssetAsync(AssetHandle handle, std::function<void(const Ref<T> &)> callback = nullptr)
    {
		std::string taskName = "AsyncAssetTask_" + std::to_string(handle);

		auto task = Application::getTaskManager()->getTask<Ref<T>>(taskName);
		if (task)
		{
			if (task->getStatus() == Task<Ref<T>>::Status::Completed)
			{
				if (callback) callback(*task->getResult());
				return task;
			}
			return task;
		}

		// Create a new task since none exists
		task = CreateRef<Task<Ref<T>>>(taskName,
		   [handle]() -> Ref<T>
		   {
			   auto asset = ProjectManager::getEditorAssetManager()->getAsset(handle);
			   return std::static_pointer_cast<T>(asset);
		   });

		if (callback) task->setCallback(callback);

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

    static bool isAssetLoaded(AssetHandle handle) 
    {
        return ProjectManager::getEditorAssetManager()->isAssetLoaded(handle);
    }

    static bool removeAsset(AssetHandle handle) 
    {
		return ProjectManager::getEditorAssetManager()->removeAsset(handle);   
    }

    static void unloadAllAssets() 
    {
        return ProjectManager::getEditorAssetManager()->unloadAllAssets();
    }

    static void serializeAssetDirectory() 
    {
        ProjectManager::getEditorAssetManager()->serializeAssetRegistry();
    }
};
}