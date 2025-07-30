#include "environment_panel.h"

#include <imgui.h>
#include <IconsFontAwesome5.h>
#include "asset_management/asset.h"
#include "asset_management/asset_manager.h"
#include "asset_management/editor_asset_manager.h"
#include "core/helpers/imgui.h"

namespace sky 
{
bool EnvironmentPanel::m_isOpen = false;

void EnvironmentPanel::render()
{
    if (!m_isOpen) return;

    ImGui::Begin(ICON_FA_CLOUD_SUN " Environment", &m_isOpen);

    helper::imguiCollapsingHeaderStyle("Sky", [this]() {
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
                        m_context->setEnvironment({
                            .skyboxHandle = handle
                        });
                        m_context->useEnvironment();
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