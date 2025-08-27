#pragma once

#include <skypch.h>

#include <glm/glm.hpp>

namespace sky
{
class EditorInfo
{
  public:
    static EditorInfo &get()
    {
        static EditorInfo instance;
        return instance;
    }

    glm::vec2 viewportSize;
    glm::vec2 gameViewportSize;
    glm::vec2 viewportMousePos;
    bool viewportIsFocus;

  private:
    EditorInfo() = default;
};

class EditorSettings
{
  public:
    static EditorSettings &get()
    {
        static EditorSettings instance;
        return instance;
    }

    bool showGrid = true;
    bool snapToGrid = false;
    float gridSize = 1.0f;
    float rotationSnapAngle = 15.0f; // degrees
    bool enableGizmos = true;

  private:
    EditorSettings() = default;
};
}