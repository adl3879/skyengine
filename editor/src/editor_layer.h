#pragma once

#include <core/layer.h>
#include <skypch.h>

namespace sky
{
class EditorLayer : public Layer
{
  public:
    EditorLayer();
    ~EditorLayer() override = default;

    void onAttach() override;
    void onDetach() override;
    void onUpdate(float dt) override;
    void onFixedUpdate(float dt) override;
    void onImGuiRender() override;
}; 
} // namespace sky
