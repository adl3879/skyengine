#include "viewport_panel.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <IconsFontAwesome5.h>
#include <ImGuizmo.h>
#include <tracy/Tracy.hpp>

#include "core/application.h"
#include "asset_management/asset_manager.h"
#include "asset_management/texture_importer.h"
#include "core/editor.h"
#include "core/events/key_codes.h"
#include "scene/components.h"
#include "scene/entity.h"
#include "core/events/input.h"
#include "core/helpers/image.h"
#include "scene/scene_manager.h"

namespace sky
{
void ViewportPanel::render()
{
    drawSceneViewport();
    drawGameViewport();
}

void ViewportPanel::drawSceneViewport()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("Scene");

    auto selectedEntity = m_context->getSelectedEntity();
    if (selectedEntity.hasComponent<CameraComponent>())
    {
        ImVec2 content_region = ImGui::GetContentRegionAvail();
        ImVec2 child_size = ImVec2(420, 280); // width, height
        uint32_t gap = 20; // gap between the viewport and the child window
        
        ImVec2 child_pos = ImVec2(
            content_region.x - gap - child_size.x,  // right edge
            content_region.y - gap - child_size.y   // bottom edge
        );
        
        auto cursorPos = ImGui::GetCursorPos();
        ImGui::SetCursorPos({cursorPos.x + child_pos.x, cursorPos.y + child_pos.y});

        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.05f, 0.05f, 0.05f, 1.0f)); 
        
        ImGui::BeginChild("BottomRightChild", child_size, true, ImGuiWindowFlags_NoScrollbar);
        
        ImGui::Text("Camera Preview");
        // ImGui::Separator();
        ImVec2 image_size = ImGui::GetContentRegionAvail();
        auto cam = selectedEntity.getComponent<CameraComponent>().camera;
        ImGui::Image(cam.getPreviewImage(), image_size,
            /*vertical flip*/ {0, 1}, {1, 0});

        m_context->sceneViewportIsVisible = ImGui::IsWindowAppearing() || !ImGui::IsWindowCollapsed();
        
        ImGui::EndChild();

        ImGui::PopStyleColor();

        ImGui::SetCursorPos({cursorPos.x, cursorPos.y}); // Reset cursor position
    }

    auto viewportOffset = ImGui::GetCursorPos();
    auto viewportSize = ImGui::GetContentRegionAvail();
    auto windowSize = ImGui::GetWindowSize();
    auto miniBound = ImGui::GetWindowPos();
    miniBound.x += viewportOffset.x;
    miniBound.y += viewportOffset.y;

    auto maxBound = ImVec2(miniBound.x + windowSize.x, miniBound.y + windowSize.y);
    m_viewportBounds[0] = {miniBound.x, miniBound.y};
    m_viewportBounds[1] = {maxBound.x, maxBound.y};

    auto [mx, my] = ImGui::GetMousePos();
    mx -= m_viewportBounds[0].x;
    my -= m_viewportBounds[0].y;

    auto mainWindowSize = Application::getWindow()->getExtent();
    auto ratioX = mainWindowSize.width / windowSize.x;
    auto ratioY = mainWindowSize.height / windowSize.y;

    bool isMouseInViewport = mx >= 0 && mx <= viewportSize.x && my >= 0 && my <= viewportSize.y;

    // set editor viewport info
    EditorInfo::get().viewportMousePos = {mx * ratioX, (windowSize.y - my) * ratioY};
    EditorInfo::get().viewportSize = {viewportSize.x, viewportSize.y};
    EditorInfo::get().viewportIsFocus = isMouseInViewport && !m_itemIsDraggedOver;

    ImGui::Image(Application::getRenderer()->getSceneImage(), 
        viewportSize, 
        /*vertical flip*/ {0, 1}, {1, 0});

    if (ImGui::BeginDragDropTarget())
    {
        handleViewportDrop();
        ImGui::EndDragDropTarget(); 
        m_itemIsDraggedOver = true;
    } else m_itemIsDraggedOver = false;

    // controls
    ImGui::SetItemAllowOverlap();
    ImGui::SetCursorPos({10, 40});

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{8, 0});
    ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2{0.5, 0.5});

    // color to show it is selected
    drawControls(ICON_FA_MOUSE_POINTER, 
        "Select", 
        m_gizmoType == -1, 
        [&]{ m_gizmoType = -1; });
    ImGui::SameLine();
    drawControls(ICON_FA_ARROWS_ALT, 
        "Move", 
        m_gizmoType == ImGuizmo::OPERATION::TRANSLATE, 
        [&]{ m_gizmoType = ImGuizmo::OPERATION::TRANSLATE; });
    ImGui::SameLine();
    drawControls(ICON_FA_SYNC_ALT, 
        "Rotate",  
        m_gizmoType == ImGuizmo::OPERATION::ROTATE, 
        [&]{ m_gizmoType = ImGuizmo::OPERATION::ROTATE; });
    ImGui::SameLine();
    drawControls(ICON_FA_EXPAND_ARROWS_ALT, 
        "Scale", 
        m_gizmoType == ImGuizmo::OPERATION::SCALE, 
        [&] { m_gizmoType = ImGuizmo::OPERATION::SCALE; });
    ImGui::SameLine();

    ImGui::PopStyleVar(2);

    drawGizmo({viewportSize.x, viewportSize.y}); 
    // drawCameraRect();

    ImGui::End();
    ImGui::PopStyleVar();
}

void ViewportPanel::drawGameViewport()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("Game");

    auto viewportSize = ImGui::GetContentRegionAvail();

    EditorInfo::get().gameViewportSize = {viewportSize.x, viewportSize.y};

    ImGui::Image(Application::getRenderer()->getGameImage(), 
        viewportSize, 
        /*vertical flip*/ {0, 1}, {1, 0});

    m_context->gameViewportIsVisible = ImGui::IsWindowAppearing() || !ImGui::IsWindowCollapsed();
    
    ImGui::End();
    ImGui::PopStyleVar();
}

void ViewportPanel::onEvent(Event &e) 
{
	EventDispatcher dispatcher(e);
    dispatcher.dispatch<KeyPressedEvent>(SKY_BIND_EVENT_FN(ViewportPanel::onKeyPressed));
}

bool ViewportPanel::onKeyPressed(KeyPressedEvent &e) 
{
    switch (e.getKeyCode())
    {
        case Key::Q: m_gizmoType = -1; break;
        case Key::W: m_gizmoType = ImGuizmo::OPERATION::TRANSLATE; break;
        case Key::E: m_gizmoType = ImGuizmo::OPERATION::ROTATE; break;
        case Key::R: m_gizmoType = ImGuizmo::OPERATION::SCALE; break;
        default: break;
    }
    return true;
}

const glm::vec3 ViewportPanel::getRayIntersectionPoint()
{
	auto windowSize = ImGui::GetWindowSize();
	auto viewportOffset = ImGui::GetCursorPos();
	auto miniBound = ImGui::GetWindowPos();
	miniBound.x += viewportOffset.x;
	miniBound.y += viewportOffset.y;

	auto maxBound = ImVec2(miniBound.x + windowSize.x, miniBound.y + windowSize.y);
	m_viewportBounds[0] = {miniBound.x, miniBound.y};
	m_viewportBounds[1] = {maxBound.x, maxBound.y};

	auto [mx, my] = ImGui::GetMousePos();
	mx -= m_viewportBounds[0].x;
	my -= m_viewportBounds[0].y;

    auto editorCamera = SceneManager::get().getEditorCamera();
	auto proj = editorCamera->getProjectionMatrix();
	auto view = editorCamera->getViewMatrix();
	auto camPos = editorCamera->getPosition();

	float x_ndc = (2.0f * mx / windowSize.x) - 1.0f;
	float y_ndc = 1.0f - (2.0f * -my / windowSize.y);

	glm::vec4 rayClip = glm::vec4(x_ndc, y_ndc, 1.0, 1.0);
	glm::vec4 rayEye = glm::inverse(proj) * rayClip;
	rayEye = glm::vec4(-rayEye.x, rayEye.y, 1.0, 0.0);
	glm::vec3 rayWorld = glm::normalize(glm::vec3(glm::inverse(view) * rayEye));

	float t = -camPos.y / rayWorld.y;
    return glm::vec3{camPos.x, camPos.y, camPos.z} + t * rayWorld;
}

void ViewportPanel::handleViewportDrop() 
{
    if (const auto *payload = ImGui::GetDragDropPayload())
    {
        auto renderer = Application::getRenderer();
        fs::path path = (const char *)payload->Data;
		auto assetType = getAssetTypeFromFileExtension(path.extension());
        if (assetType != AssetType::Mesh) return;

		auto handle = AssetManager::getOrCreateAssetHandle(path, assetType);
        Ref<Model> model;
        if (AssetManager::isAssetLoaded(handle))
        {
            model = AssetManager::getAsset<Model>(handle);
        }
        else
        {
            if (!renderer->isTempModelLoaded(path))
            {
				const auto &fullPath = ProjectManager::getConfig().getAssetDirectory() / path;
				AssimpModelLoader loader(fullPath);
				std::vector<MeshID> meshIds;
				for (auto &mesh : loader.getMeshes())
				{
                    auto material = Material{};
                    auto albedoPath = ProjectManager::getConfig().getAssetDirectory() / mesh.materialPaths.albedoTexture;
                    // load just albedo for performance
                    material.albedoTexture = helper::loadImageFromTexture(
                        TextureImporter::loadTexture(albedoPath), 
                        VK_FORMAT_R8G8B8A8_SRGB, 
                        VK_IMAGE_USAGE_SAMPLED_BIT, 
                        true);
					mesh.mesh.material = renderer->addMaterialToCache(material);
					auto id = renderer->addMeshToCache(mesh.mesh);
					meshIds.push_back(id);
				}
				model = CreateRef<Model>(meshIds);
                renderer->addTempModel(path, model);
            }
            else
            {
                model = renderer->getTempModel(path);
            }
        }

        const auto intersectionPoint = getRayIntersectionPoint();
		glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), intersectionPoint);
        renderer->drawModel(model, translationMatrix);
    }

    if (const auto *payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
    {
        fs::path path = (const char *)payload->Data;
        auto assetType = getAssetTypeFromFileExtension(path.extension());

        switch (assetType)
        {
            case AssetType::Mesh:
            {
                auto handle = AssetManager::getOrCreateAssetHandle(path, assetType);
                auto entity = m_context->createEntity(path.stem().string());
                entity.addComponent<ModelComponent>().handle = handle;
                entity.getComponent<TransformComponent>().transform.setPosition(getRayIntersectionPoint());
                m_context->getSceneGraph()->parentEntity(m_context->getRootEntity(), entity);
                m_context->setSelectedEntity(entity);
				break;
            }
            case AssetType::Scene: break;
            case AssetType::Material: break;
            default: break;
        }
    }
}

void ViewportPanel::drawControls(const char *icon, const char *tooltip, bool isActive, std::function<void()> action)
{
    float buttonSize = 42;
    constexpr auto activeColor = ImVec4(0.0f, 0.447f, 0.776f, 1.0f);

    // change color of selected button
    if (isActive)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, activeColor);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, activeColor);
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.2f, 0.2f, 0.2f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.3f, 0.3f, 0.3f, 1.0f});
    }

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
    if (ImGui::Button(icon, {buttonSize, buttonSize})) action();
    ImGui::PopStyleVar();

    if (ImGui::IsItemHovered()) m_isControlPressed = true;

    ImGui::PopStyleColor(2);
}

void ViewportPanel::drawGizmo(const glm::vec2 &size) 
{
    auto selectedEntity = m_context->getSelectedEntity();

    auto editorCamera = SceneManager::get().getEditorCamera();
    auto cameraView = editorCamera->getView();
    auto cameraProjection = editorCamera->getProjection();

    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist();
    ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, size.x, size.y);

    if (selectedEntity && m_gizmoType != -1)
    {
        auto &tc = selectedEntity.getComponent<TransformComponent>().transform;

        glm::mat4 transform = tc.getWorldMatrix();

        // --- Snapping ---
        bool snap = Input::isKeyPressed(Key::LeftControl);
        float snapValue = m_gizmoType == ImGuizmo::OPERATION::ROTATE ? 45.0f : 0.5f;
        float snapValues[3] = {snapValue, snapValue, snapValue};

        ImGuizmo::Manipulate(
            glm::value_ptr(cameraView),
            glm::value_ptr(cameraProjection),
            (ImGuizmo::OPERATION)m_gizmoType,
            ImGuizmo::MODE::WORLD, 
            glm::value_ptr(transform),
            nullptr,
            snap ? snapValues : nullptr
        );

        if (ImGuizmo::IsUsing())
        {
            // If entity has a parent, we must convert back to local
            glm::mat4 parentWorld = glm::mat4(1.0f);
            if (selectedEntity.hasComponent<RelationshipComponent>())
            {
                auto &rel = selectedEntity.getComponent<RelationshipComponent>();
                if (rel.parent != NULL_UUID)
                {
                    auto parentEntity = m_context->getEntityFromUUID(rel.parent);
                    auto &parentTransform = parentEntity.getComponent<TransformComponent>().transform;
                    parentWorld = parentTransform.getWorldMatrix();
                }
            }

            // Local = inverse(parentWorld) * newWorld
            glm::mat4 newLocalTransform = glm::inverse(parentWorld) * transform;

            // 4. Decompose to position, rotation, scale
            glm::vec3 pos, scale;
            glm::quat rot;
            math::DecomposeMatrix(newLocalTransform, pos, rot, scale);

            tc.setPosition(pos);
            if (m_gizmoType != ImGuizmo::SCALE) tc.setRotation(rot);
            tc.setScale(scale);
        } 
    }
}
} // namespace sky