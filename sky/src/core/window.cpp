#include "window.h"

#include "core/events/input.h"
#include "core/application.h"

namespace sky
{
Window::Window(int width, int height, std::string name)
{
    m_data.windowName = name;
    m_data.width = width;
    m_data.height = height;
    initWindow();
}

Window::~Window()
{
    glfwDestroyWindow(m_window);
    glfwTerminate();
}

void Window::createWindowSurface(VkInstance instance, VkSurfaceKHR *surface)
{
    if (glfwCreateWindowSurface(instance, m_window, nullptr, surface) != VK_SUCCESS)
    {
        // TODO!  remove exception
        throw std::runtime_error("failed to create window surface!");
    }
}

void Window::framebufferResizeCallback(GLFWwindow *window, int width, int height)
{
    auto wnd = reinterpret_cast<WindowData *>(glfwGetWindowUserPointer(window));
    // m_framebufferResized = true;
    wnd->width = width;
    wnd->height = height;
}

void Window::initWindow()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    m_window = glfwCreateWindow(m_data.width, m_data.height, m_data.windowName.c_str(), nullptr, nullptr);

    glfwSetWindowUserPointer(m_window, &m_data);
    glfwSetFramebufferSizeCallback(m_window, framebufferResizeCallback);

    // set key callback
    glfwSetKeyCallback(m_window,
                       [](GLFWwindow *window, int key, int scancode, int action, int mods)
                       {
                           WindowData &data = *(WindowData *)glfwGetWindowUserPointer(window);

                           switch (action)
                           {
                               case GLFW_PRESS:
                               {
                                   KeyPressedEvent event(key, 0);
                                   data.eventCallback(event);
                                   break;
                               }
                               case GLFW_RELEASE:
                               {
                                   KeyReleasedEvent event(key);
                                   data.eventCallback(event);
                                   break;
                               }
                               case GLFW_REPEAT:
                               {
                                   KeyPressedEvent event(key, true);
                                   data.eventCallback(event);
                                   break;
                               }
                           }
                       });

    glfwSetCharCallback(m_window,
                        [](GLFWwindow *window, unsigned int keycode)
                        {
                            WindowData &data = *(WindowData *)glfwGetWindowUserPointer(window);

                            KeyTypedEvent event(keycode);
                            data.eventCallback(event);
                        });

    // set mouse button callback
    glfwSetMouseButtonCallback(m_window,
                               [](GLFWwindow *window, int button, int action, int mods)
                               {
                                   WindowData &data = *(WindowData *)glfwGetWindowUserPointer(window);

                                   switch (action)
                                   {
                                       case GLFW_PRESS:
                                       {
                                           MouseButtonPressedEvent event(button);
                                           data.eventCallback(event);
                                           break;
                                       }
                                       case GLFW_RELEASE:
                                       {
                                           MouseButtonReleasedEvent event(button);
                                           data.eventCallback(event);
                                           break;
                                       }
                                   }
                               });

    glfwSetScrollCallback(m_window,
                          [](GLFWwindow *window, double xOffset, double yOffset)
                          {
                              WindowData &data = *(WindowData *)glfwGetWindowUserPointer(window);

                              MouseScrolledEvent event((float)xOffset, (float)yOffset);
                              data.eventCallback(event);
                          });

    glfwSetCursorPosCallback(m_window,
                             [](GLFWwindow *window, double xPos, double yPos)
                             {
                                 WindowData &data = *(WindowData *)glfwGetWindowUserPointer(window);

                                 MouseMovedEvent event((float)xPos, (float)yPos);
                                 data.eventCallback(event);
                             });
}

bool Input::isKeyPressed(const KeyCode key)
{
    auto *window = static_cast<GLFWwindow *>(Application::getWindow()->getGLFWwindow());
    auto state = glfwGetKey(window, static_cast<int32_t>(key));
    return state == GLFW_PRESS;
}

bool Input::isMouseButtonPressed(const MouseCode button)
{
    auto *window = static_cast<GLFWwindow *>(Application::getWindow()->getGLFWwindow());
    auto state = glfwGetMouseButton(window, static_cast<int32_t>(button));
    return state == GLFW_PRESS;
}

glm::vec2 Input::getMousePosition()
{
    auto *window = static_cast<GLFWwindow *>(Application::getWindow()->getGLFWwindow());
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    return {(float)xpos, (float)ypos};
}

float Input::getMouseX() { return getMousePosition().x; }

float Input::getMouseY() { return getMousePosition().y; }

glm::vec2 Input::getMouseDelta()
{
    const auto mousePos = getMousePosition();

    static double lastX = mousePos.x, lastY = mousePos.y;
    glm::vec2 delta = {(float)(mousePos.x - lastX), (float)(mousePos.y - lastY)};
    lastX = mousePos.x;
    lastY = mousePos.y;

    return delta;
}
} // namespace sky