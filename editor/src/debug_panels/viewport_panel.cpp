#include "viewport_panel.h"

#include <imgui.h>
#include <imgui_internal.h>
#include "core/application.h"
#include "asset_management/asset_manager.h"
#include "scene/components.h"
#include "scene/entity.h"
#include "scene/scene_manager.h"

namespace sky
{
void ViewportPanel::render() 
{
    auto renderer = Application::getRenderer();

    for (const auto &scene : SceneManager::get().getOpenedScenes())
    {	
        bool show = true;
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::Begin(scene.string().c_str(), &show);
		auto viewportSize = ImGui::GetContentRegionAvail();
		m_context->setViewportInfo({
			.size = {viewportSize.x, viewportSize.y},
			.isFocus = ImGui::IsWindowFocused() && ImGui::IsWindowHovered(),
		});
		ImGui::Image(renderer->getDrawImageId(), viewportSize);

		if (ImGui::BeginDragDropTarget())
		{
			handleViewportDrop();
			ImGui::EndDragDropTarget(); 
		}

        if (scene != "[empty]")
        {
			bool isCurrentlyFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
			static std::string wasFocused{};
			if (isCurrentlyFocused && wasFocused != scene.string())
			{
				wasFocused = scene.string();
                SceneManager::get().openScene(scene);
			}
			if (show == false) SceneManager::get().closeScene(scene);
        }
		
		ImGui::End();
		ImGui::PopStyleVar();
    }
}

void ViewportPanel::handleViewportDrop() 
{
    if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
    {
        fs::path path = (const char *)payload->Data;
        auto assetType = getAssetTypeFromFileExtension(path.extension());

        switch (assetType)
        {
            case AssetType::Mesh:
            {
                auto handle = AssetManager::getOrCreateAssetHandle(path, assetType);
                //auto asset = AssetManager::loadAssetAsync<Model>(handle);

                auto entity = m_context->createEntity(path.stem().string());
                entity.addComponent<ModelComponent>().handle = handle;
                m_context->setSelectedEntity(entity);
				break;
            }
            case AssetType::Scene: break;
            case AssetType::Material: break;
            default: break;
        }
    }

}
} // namespace sky