#include "application.h"

#include <skypch.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

namespace sky {
Ref<Window> Application::m_window = nullptr;

Application::Application() 
{
    m_window = CreateRef<Window>(1280, 720, "Sky");

	m_window->setEventCallback(std::bind(&Application::onEvent, this, std::placeholders::_1));
}

Application::~Application() {}

void Application::pushLayer(Layer *layer) 
{
    m_layerStack.pushLayer(layer);
    layer->onAttach();
}

void Application::pushOverlay(Layer *layer) 
{
    m_layerStack.pushOverlay(layer);
    layer->onAttach();
}

void Application::run() 
{
    auto currentTime = std::chrono::high_resolution_clock::now();

    while (!m_window->shouldClose()) 
    {
        glfwPollEvents();

        auto newTime = std::chrono::high_resolution_clock::now();
        float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
        currentTime = newTime;

        // imgui new frame
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // some imgui UI to test
        ImGui::ShowDemoWindow();

        // make imgui calculate internal draw structures
        ImGui::Render();

        for (Layer *layer : m_layerStack) 
        {
            layer->onUpdate(0.0f);
        }
    }
}

void Application::onEvent(Event &e)
{
    EventDispatcher dispatcher(e);

    for (auto it = m_layerStack.rbegin(); it != m_layerStack.rend(); ++it)
    {
        if (e.handled) break;
        (*it)->onEvent(e);
    }
}
} // namespace sky