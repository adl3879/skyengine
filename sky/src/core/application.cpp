#include "application.h"

#include <skypch.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include "renderer/model_loader.h"

namespace sky {
Ref<Window> Application::m_window = nullptr;

Application::Application() 
{
    m_window = CreateRef<Window>(1280, 720, "Sky");
	m_window->setEventCallback(std::bind(&Application::onEvent, this, std::placeholders::_1));

    m_gfxDevice = CreateRef<gfx::Device>(*m_window);
    m_renderer = CreateRef<SceneRenderer>(*m_gfxDevice, m_meshCache);
    
    auto extent = m_window->getExtent();
    m_renderer->init({extent.width, extent.height});

    AssimpModelLoader modelLoader("res/models/monkey.glb");
    auto meshId = m_meshCache.addMesh(*m_gfxDevice, modelLoader.getMeshes()[0]);
    m_renderer->addDrawCommand(MeshDrawCommand{
        .meshId = meshId, .modelMatrix = glm::mat4{}
    });
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
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // some imgui UI to test
        ImGui::ShowDemoWindow();

        // make imgui calculate internal draw structures
        ImGui::Render();

        auto cmd = m_gfxDevice->beginFrame();
        m_renderer->render(cmd);
        m_gfxDevice->endFrame(cmd, m_renderer->getDrawImage());
        
        if (m_gfxDevice->needsSwapchainRecreate())
        {
            auto extent = m_window->getExtent();
            m_gfxDevice->recreateSwapchain(cmd, extent.width, extent.height);
        }

        for (Layer *layer : m_layerStack) 
        {
            layer->onUpdate(0.0f);
        }
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