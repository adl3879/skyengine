#pragma once

#include <skypch.h>
#include "scene/entity.h"
#include "renderer/material.h"

namespace sky
{
enum class InspectorPanelView
{
	Default,
	MaterialEditor,
};

class InspectorPanel
{
  public:
	struct MaterialContext 
	{ 
		bool isCustom = false;
		AssetHandle assetHandle = NULL_UUID; 
		MaterialID materialId = NULL_MATERIAL_ID;
	};

  public:
	InspectorPanel();

	void reset();
	void render();
	void setContext(Ref<Scene> ctx) { m_context = ctx; }
	void setMaterialContext(MaterialContext ctx) { m_materialContext = ctx; }
	void openView(InspectorPanelView view);

  private:
	void drawDefaultView();
	void drawMaterialEditor();

	void drawTransformComponent();
	void drawMeshComponent();
	void drawDirectionalLightComponent();
	void drawPointLightComponent();
	void drawSpotLightComponent();
	void drawSpriteRendererComponent();

  private:
	Ref<Scene> m_context;
	bool m_renameRequested = false;
	InspectorPanelView m_view = InspectorPanelView::Default;
	InspectorPanelView m_previousView;

	MaterialContext m_materialContext;

    std::unordered_map<std::string, ImageID> m_icons;
};
}