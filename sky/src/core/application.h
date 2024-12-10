#pragma once

#include "layer_stack.h"
#include "window.h"
#include "graphics/vulkan/vk_device.h"
#include "renderer/scene_renderer.h"
#include "tasks/task_manager.h"
#include "math/fps_counter.h"

int main(int argc, char** argv);

namespace sky 
{
class Application
{
  public:
    Application();
    virtual ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    static constexpr int WIDTH = 1400;
    static constexpr int HEIGHT = 1000;
    static constexpr const char* TITLE = "Sky Engine";

    void onEvent(Event &e);

    void pushLayer(Layer *layer);
    void pushOverlay(Layer *layer);

    static Ref<Window> getWindow() { return m_window; }
    static Ref<SceneRenderer> getRenderer() { return m_renderer; }
    static Ref<TaskManager> getTaskManager() { return m_taskManager; }
    static std::vector<fs::path> &getDroppedFiles() { return m_droppedFiles; }
    static void quit();
    static double getFPS() { return m_fps.getFPS(); }

  private:
    void run();

  private:
    friend int ::main(int argc, char** argv);

  private:
    inline static bool m_isRunning = true;
    inline static std::vector<fs::path> m_droppedFiles;
    inline static FPSCounter m_fps;

    LayerStack m_layerStack{};
    static Ref<Window> m_window;
	static Ref<gfx::Device> m_gfxDevice;
    static Ref<SceneRenderer> m_renderer;
    static Ref<TaskManager> m_taskManager;
};

Application *CreateApplication();
} // namespace sky