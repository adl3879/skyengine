#include "editor_layer.h"

namespace sky
{
EditorLayer::EditorLayer() {}

void EditorLayer::onAttach() 
{
    std::cout << "EditorLayer::onAttach" << std::endl;
}

void EditorLayer::onDetach() {}

void EditorLayer::onUpdate(float dt) {}

void EditorLayer::onFixedUpdate(float dt) {}

void EditorLayer::onImGuiRender() {}
} // namespace sky