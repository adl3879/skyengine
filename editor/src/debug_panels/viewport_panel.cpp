#include "viewport_panel.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <IconsFontAwesome5.h>
#include <ImGuizmo.h>
#include <tracy/Tracy.hpp>

#include "core/application.h"
#include "asset_management/asset_manager.h"
#include "asset_management/mesh_importer.h"
#include "asset_management/texture_importer.h"
#include "scene/components.h"
#include "scene/entity.h"
#include "scene/scene_manager.h"
#include "core/events/input.h"
#include "core/helpers/image.h"

namespace sky
{
void ViewportPanel::render() 
{
    ZoneScopedN("Viewport render");
    auto renderer = Application::getRenderer();

    for (const auto &scene : SceneManager::get().getOpenedScenes())
    {	
        bool show = true;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::Begin(scene.string().c_str(), &show);

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

		m_context->setViewportInfo({
			.size = {viewportSize.x, viewportSize.y},
			.mousePos = {mx * ratioX, my * ratioY},
			.isFocus = isMouseInViewport && !m_itemIsDraggedOver,
		});
		ImGui::Image(renderer->getDrawImageId(), viewportSize);
 
		if (ImGui::BeginDragDropTarget())
		{
			handleViewportDrop();
			ImGui::EndDragDropTarget(); 
            m_itemIsDraggedOver = true;
		} else m_itemIsDraggedOver = false;

		if (scene != "[empty]")
		{
			bool isFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
			ImVec2 mousePos = ImGui::GetMousePos();
			ImVec2 windowPos = ImGui::GetWindowPos();
			ImVec2 windowSize = ImGui::GetWindowSize();

			// Calculate the title bar region (assume fixed title bar height)
			const float titleBarHeight = ImGui::GetFrameHeight();
			bool isMouseOverTitleBar = mousePos.x >= windowPos.x && mousePos.x <= (windowPos.x + windowSize.x) &&
									   mousePos.y >= windowPos.y && mousePos.y <= (windowPos.y + titleBarHeight);

			if (isFocused && isMouseOverTitleBar && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
			{
				SceneManager::get().openScene(scene);
			}
			if (show == false) SceneManager::get().closeScene(scene);
		}

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
        //TDOD: FIX! 
        //updateCameraManipulator({viewportSize.x, viewportSize.y});

		ImGui::End();
		ImGui::PopStyleVar();
    }
}

void ViewportPanel::onEvent(Event &e) 
{
    if (!m_context->getViewportInfo().isFocus) return;

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

const glm::vec3 &ViewportPanel::getRayIntersectionPoint()
{
    // TODO: insert return statement here
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

	auto proj = m_context->getEditorCamera().getProjectionMatrix();
	auto view = m_context->getEditorCamera().getViewMatrix();
	auto camPos = m_context->getEditorCamera().getPosition();

	float x_ndc = (2.0f * mx / windowSize.x) - 1.0f;
	float y_ndc = 1.0f - (2.0f * -my / windowSize.y);

	glm::vec4 rayClip = glm::vec4(x_ndc, y_ndc, 1.0, 1.0);
	glm::vec4 rayEye = glm::inverse(proj) * rayClip;
	rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0, 0.0);
	glm::vec3 rayWorld = glm::normalize(glm::vec3(glm::inverse(view) * rayEye));

	float t = -camPos.y / rayWorld.y;
	glm::vec3 intersectionPoint = glm::vec3{camPos.x, camPos.y, camPos.z} + t * rayWorld;

    return intersectionPoint;
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
                entity.getComponent<TransformComponent>().setPosition(getRayIntersectionPoint());
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

void ViewportPanel::updateCameraManipulator(const ImVec2 &size)
{
    glm::mat4 cameraView = m_context->getCamera().getView();
    float *cameraViewPtr = glm::value_ptr(cameraView);

    // Add a small deadzone to prevent micro-adjustments
    static glm::mat4 lastView = cameraView;
    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist();
    ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, size.x, size.y);

    ImGuizmo::ViewManipulate(cameraViewPtr,
                             10.0f,                   // Distance
                             ImVec2(size.x + 400, size.y - 100), // Position
                             ImVec2(200, 200),        // Size
                             0x10101010               // Background color (slight gray to see the widget)
    );

    // Only update if the change is significant
    if (!glm::all(glm::epsilonEqual(glm::vec3(cameraView[3]), glm::vec3(lastView[3]), 0.0001f)))
    {
        lastView = cameraView;
        m_context->getCamera().setView(cameraView);
    }
}

void ViewportPanel::drawGizmo(const glm::vec2 &size) 
{
    auto selectedEntity = m_context->getSelectedEntity();

    glm::mat4 cameraView = m_context->getCamera().getView();
    glm::mat4 cameraProjection = m_context->getCamera().getProjection();

    cameraView[0][1] = -cameraView[0][1];
    cameraView[1][1] = -cameraView[1][1];
    cameraView[2][1] = -cameraView[2][1];
    cameraView[3][1] = -cameraView[3][1];

    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist();
    ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, size.x, size.y);

    if (selectedEntity && m_gizmoType != -1)
    {
        auto &tc = selectedEntity.getComponent<TransformComponent>();
        auto transform = tc.getModelMatrix();

        // Snapping
        bool snap = Input::isKeyPressed(Key::LeftControl);
        float snapValue = m_gizmoType == ImGuizmo::OPERATION::ROTATE ? 45.0f : 0.5f;
        float snapValues[3] = {snapValue, snapValue, snapValue};

        ImGuizmo::Manipulate(glm::value_ptr(cameraView), 
            glm::value_ptr(cameraProjection), 
            (ImGuizmo::OPERATION)m_gizmoType,
            ImGuizmo::MODE::LOCAL, 
            glm::value_ptr(transform),
            nullptr, 
            snap ? snapValues : nullptr);

	    if (ImGuizmo::IsUsing())
        {
            glm::mat4 localTransform = glm::mat4(transform);
           
            // Decompose local transform
            float decomposedPosition[3];
            float decomposedEuler[3];
            float decomposedScale[3];
            ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(localTransform), 
                decomposedPosition, 
                decomposedEuler,
                decomposedScale);

            const auto &localPosition = glm::vec3(decomposedPosition[0], decomposedPosition[1], decomposedPosition[2]);
            const auto &localScale = glm::vec3(decomposedScale[0], decomposedScale[1], decomposedScale[2]);

            localTransform[0] /= localScale.x;
            localTransform[1] /= localScale.y;
            localTransform[2] /= localScale.z;
            const auto &rotationMatrix = glm::mat3(localTransform);
            const glm::quat &localRotation = glm::normalize(glm::quat(rotationMatrix));

            const glm::mat4 &rotationMatrix4 = glm::mat4_cast(localRotation);
            const glm::mat4 &scaleMatrix = glm::scale(glm::mat4(1.0f), localScale);
            const glm::mat4 &translationMatrix = glm::translate(glm::mat4(1.0f), localPosition);
            const glm::mat4 &newLocalTransform = translationMatrix * rotationMatrix4 * scaleMatrix;

            tc.setPosition(localPosition);
            if (m_gizmoType != ImGuizmo::SCALE) tc.setRotation(localRotation);
            tc.setScale(localScale);
        }
    }
}
} // namespace sky