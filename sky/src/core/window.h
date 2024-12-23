#pragma once

#include <skypch.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "core/events/event.h"
#include "core/events/key_event.h"
#include "core/events/mouse_event.h"

#include <vulkan/vulkan.h>

namespace sky
{
class Window
{
  public:
    Window(int width, int height, std::string name);
    ~Window();

    Window(const Window &) = delete;
    Window &operator=(const Window &) = delete;

    GLFWwindow *getGLFWwindow() const { return m_window; }

    void destroy();

    bool shouldClose() const { return glfwWindowShouldClose(m_window); }
    VkExtent2D getExtent() const { return {static_cast<uint32_t>(m_data.width), static_cast<uint32_t>(m_data.height)}; }
    bool wasWindowResized() const { return m_framebufferResized; }
    bool isWindowMinimized() const { return m_data.width == 0 || m_data.height == 0; }
    void resetWindowResizedFlag() { m_framebufferResized = false; }
    bool isWindowMaximized();

    void setTitlebarHitTestCallback(std::function<void(bool &hit)> callback);
    void setEventCallback(const std::function<void(Event &)> &callback) { m_data.eventCallback = callback; }

    void createWindowSurface(VkInstance instance, VkSurfaceKHR *surface);

  private:
    static void framebufferResizeCallback(GLFWwindow *window, int width, int height);
    void initWindow();

    bool m_framebufferResized = false;

    struct WindowData
    {
        std::string windowName;
        int width, height;

        std::function<void(Event &)> eventCallback;
		std::function<void(bool &hit)> titleBarHitTestCallback;
    };
    WindowData m_data;

    GLFWwindow *m_window;
};
} // namespace key
