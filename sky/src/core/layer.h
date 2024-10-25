#pragma once

#include <skypch.h>

#include "core/events/event.h"

namespace sky
{
class Layer
{
  public:
    Layer(const std::string &name = "Layer");
    virtual ~Layer() = default;

    virtual void onEvent(Event &e) {}
    virtual void onAttach() {}
    virtual void onDetach() {}
    virtual void onUpdate(float dt) {}
    virtual void onFixedUpdate(float dt) {}
    virtual void onImGuiRender() {}

  protected:
    std::string debugName_;
};
} // namespace sky