#include "environment_panel.h"

#include <imgui.h>
#include <IconsFontAwesome5.h>
#include "asset_management/asset.h"
#include "asset_management/asset_manager.h"
#include "asset_management/editor_asset_manager.h"
#include "core/application.h"
#include "core/helpers/imgui.h"
#include "core/helpers/image.h"
#include "renderer/texture.h"

namespace sky 
{
bool EnvironmentPanel::m_isOpen = false;

void EnvironmentPanel::render()
{
    if (!m_isOpen) return;

    ImGui::Begin(ICON_FA_CLOUD_SUN " Environment", &m_isOpen);

    helper::imguiCollapsingHeaderStyle("Sky", []() {
        if (ImGui::BeginTable("sk_y", 2, ImGuiTableFlags_Resizable))
        {
            ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::Text("Cubemap");
			ImGui::TableNextColumn();
            ImGui::Button("Hdri path", {-1, 40});
            if (ImGui::BeginDragDropTarget())
            {
                if (const auto *payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
                {
                    fs::path path = (const char *)payload->Data;
                    auto assetType = getAssetTypeFromFileExtension(path.extension());
                    if (assetType == AssetType::TextureCube)
                    {
                        auto handle = AssetManager::getOrCreateAssetHandle(path, AssetType::TextureCube);
                        AssetManager::getAssetAsync<TextureCube>(handle, [=](const Ref<TextureCube> &hdrTex){
                            auto hdrImageId = helper::loadImageFromTexture(hdrTex, VK_FORMAT_R32G32B32A32_SFLOAT);
                            auto renderer = Application::getRenderer();
                            renderer->getIBL().setHdrImageId(hdrImageId);
                        });
                    }
                }
                ImGui::EndDragDropTarget();
            }

			ImGui::EndTable();
        }
    });

    ImGui::End();
}
}