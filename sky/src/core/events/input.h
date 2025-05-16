#pragma once

#include "key_codes.h"
#include "mouse_codes.h"

#include <glm/glm.hpp>

namespace sky
{
class Input
{
  public:
    static bool isKeyPressed(KeyCode key);

    static bool isMouseButtonPressed(MouseCode button);
    static glm::vec2 getMousePosition();
    static float getMouseX();
    static float getMouseY();
    static glm::vec2 getMouseDelta();
    static void setMousePosition(const std::uint32_t &x, const std::uint32_t &y);
    static void showMouseCursor(const bool &show);
    static void setCursorMode(CursorMode mode);
};
} // namespace sky