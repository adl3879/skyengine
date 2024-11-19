#include "application.h"

#include <skypch.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include "renderer/model_loader.h"
#include "log.h"

namespace sky {
Ref<Window>         Application::m_window       = nullptr;
Ref<gfx::Device>    Application::m_gfxDevice    = nullptr;
Ref<SceneRenderer>  Application::m_renderer     = nullptr;

Application::Application() 
{
    Log::Init();

    m_window = CreateRef<Window>(WIDTH, HEIGHT, "Sky");
	m_window->setEventCallback(std::bind(&Application::onEvent, this, std::placeholders::_1));

	m_gfxDevice = CreateRef<gfx::Device>(*m_window);
    m_renderer = CreateRef<SceneRenderer>(*m_gfxDevice);
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
        if (m_window->isWindowMinimized()) continue;

        auto newTime = std::chrono::high_resolution_clock::now();
        float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
        currentTime = newTime;

        // imgui new frame
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // some imgui UI to test
        // ImGui::ShowDemoWindow();
        for (Layer *layer : m_layerStack) layer->onImGuiRender();

        // make imgui calculate internal draw structures
        ImGui::Render();

        {
            auto cmd = m_gfxDevice->beginFrame();
            m_renderer->render(cmd);
            m_gfxDevice->endFrame(cmd, m_renderer->getDrawImage());

            if (m_gfxDevice->needsSwapchainRecreate())
            {
                auto extent = m_window->getExtent();
                m_gfxDevice->recreateSwapchain(cmd, extent.width, extent.height);
            }
        }

        for (Layer *layer : m_layerStack) layer->onUpdate(frameTime);
    }
    m_window->destroy();
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