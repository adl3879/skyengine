#include "editor_layer.h"

#include "core/application.h"
#include "scene/scene_manager.h"
#include "scene/entity.h"
#include "scene/components.h"

namespace sky
{
EditorLayer::EditorLayer()
{
    auto window = Application::getWindow();
    m_activeScene = SceneManager::get().getActiveScene();
    m_activeScene->init();

    m_gfxDevice = CreateRef<gfx::Device>(*window);
    m_renderer = CreateRef<SceneRenderer>(*m_gfxDevice, *m_activeScene);

    auto extent = window->getExtent();
    m_renderer->init({extent.width, extent.height});

    AssimpModelLoader modelLoader("res/models/monkey.glb");
    auto meshId = m_renderer->addMeshToCache(modelLoader.getMeshes()[0]);
    auto monkey = m_activeScene->createEntity("monkey");
    monkey.addComponent<MeshComponent>() = meshId;

    m_renderer->addDrawCommand(
        MeshDrawCommand{
            .meshId = meshId, 
            .modelMatrix = monkey.getComponent<TransformComponent>().getModelMatrix()
        });
}

void EditorLayer::onAttach() 
{
    std::cout << "EditorLayer::onAttach" << std::endl;
}

void EditorLayer::onDetach() 
{
    m_activeScene->cleanup();
}

void EditorLayer::onUpdate(float dt) 
{
    m_activeScene->update(dt);

    auto cmd = m_gfxDevice->beginFrame();
    m_renderer->render(cmd);
    m_gfxDevice->endFrame(cmd, m_renderer->getDrawImage());

    if (m_gfxDevice->needsSwapchainRecreate())
    {
        auto extent = Application::getWindow()->getExtent();
        m_gfxDevice->recreateSwapchain(cmd, extent.width, extent.height);
    }
}

void EditorLayer::onEvent(Event &e) 
{ 
    m_activeScene->getEditorCamera().onEvent(e);
}

void EditorLayer::onFixedUpdate(float dt) {}

void EditorLayer::onImGuiRender() {}
} // namespace sky